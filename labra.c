#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "lcd.h"
#include <avr/delay.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#define LCD_START_LINE1  0x00
#define LCD_START_LINE2  0x40

void lcd_gotoxy (unsigned char x, unsigned char y)
{
    if ( y==0 ) {
         lcd_write_ctrl(LCD_DDRAM | (LCD_START_LINE1+x));
   } else {
         lcd_write_ctrl(LCD_DDRAM | (LCD_START_LINE2+x));
}
}

#define LCD_BLINK  0x01     /*Kursorin asetus välkkymään */

void init(void) {
	/* Alustetaan komponentit */
   	/* estetään kaikki keskeytykset */
	cli();

    /* kaiutin-pinnit ulostuloksi */
    DDRE  |=  (1 << PE4) | (1 << PE5);
    /* pinni PE4 nollataan */
    PORTE &= ~(1 << PE4);
    /* pinni PE5 asetetaan */
	PORTE |=  (1 << PE5);   
        
    /* ajastin nollautuu, kun sen ja OCR1A rekisterin arvot ovat samat */
    TCCR1A &= ~( (1 << WGM11) | (1 << WGM10) );
    TCCR1B |=    (1 << WGM12);
    TCCR1B &=   ~(1 << WGM13);

    /* salli keskeytys, jos ajastimen ja OCR1A rekisterin arvot ovat samat */
    TIMSK |= (1 << OCIE1A);

    /* asetetaan OCR1A rekisterin arvoksi 0x3e (~250hz) */
    OCR1AH = 0x00;
    OCR1AL = 0x3e;

    /* käynnistä ajastin ja käytä kellotaajuutena (16 000 000 / 1024) Hz */
    TCCR1B |= (1 << CS12) | (1 << CS10);

	/* näppäin pinnit sisääntuloksi */
	DDRA &= ~(1 << PA0);
	DDRA &= ~(1 << PA2);
	DDRA &= ~(1 << PA4);

	/* rele/led pinni ulostuloksi */
	DDRA |= (1 << PA6);

	/* lcd-näytön alustaminen */
	lcd_init();
	lcd_write_ctrl(LCD_ON);
	lcd_write_ctrl(LCD_CLEAR);
		
}

void nayton_tulostus(int y, char kentta[10][10], int ammukset, int ennatys)
/*/Tulostaa pelilaudan LCD-näytölle sekä jäljellä olevien ammusten määrän*/
{
	int i = 0, j = 0, kymmenet = 0;
	lcd_gotoxy(0,0);
	if(y > 0 && y < 10)
		y--;
    for (i=y; i<=y+1; i++)
		{ 
		
		lcd_write_data('0'+i);
		for (j=0; j<10; j++)
			lcd_write_data(kentta[i][j]);
		if(i==y){								/*/Ammusten lkm:n tulostus yläriville*/
			lcd_write_data(' ');
			lcd_write_data('A');
			lcd_write_data('=');

			if (ammukset < 9) {
				lcd_write_data('0' + ammukset);
				lcd_write_data(' ');
			}
			if (ammukset > 9) {
				kymmenet = ammukset/10;
				lcd_write_data('0' + kymmenet);
				kymmenet = kymmenet * 10;
				kymmenet = ammukset - kymmenet;
				lcd_write_data('0' + kymmenet);
			}
		}
		if(i==(y+1)){								/*/Ennätyksen tulostus alariville*/
			kymmenet = 0;
			lcd_write_data(' ');
			lcd_write_data(' ');
			lcd_write_data(' ');

			if (ennatys < 9) {
				lcd_write_data(' ');
				lcd_write_data('0' + ennatys);
			}
			if (ennatys > 9) {
				kymmenet = ennatys/10;
				lcd_write_data('0' + kymmenet);
				kymmenet = kymmenet * 10;
				kymmenet = ennatys - kymmenet;
				lcd_write_data('0' + kymmenet);
			}
		}
		lcd_gotoxy(0,1);
	}
}
void kentan_taytto(char kentta[10][10])
/*/Pelaajalle näytettävän pelilaudan alkutäyttö*/
{
	int i = 0, j = 0;
	 for (i=0; i<10; i++){
	 	for(j=0; j<10; j++){
				kentta[i][j] = ('^');
		}
	}

}

void aikentan_taytto(char aikentta[10][10])
/*/Tietokoneen pelilaudan alkutäyttö*/
{
	int i = 0, j = 0;
	for (i=0; i<10; i++){
	 	for(j=0; j<10; j++){
				aikentta[i][j] = ('^');
		}
	}

}

int tarkista_nappulat_x(int x)
/*/Tarkistaa onko sivuillemenonappuloita B2 tai B4 painettu 
ja liikuttaa kursoria vasemmalle tai oikealle*/
{
			if (!(PINA & (1 << PA3)))
				{
				x = x+1;
				if(x==11)
					x = 1;
				sei();
				_delay_ms(150);
				cli();
				_delay_ms(50);
				}
			if (!(PINA & (1 << PA1)))
				{
				x = x-1;
				if(x==0)
					x = 10;
				sei();
				_delay_ms(150);
				cli();
				_delay_ms(50);
				}
			return x;
}

int tarkista_nappulat_y(int y, char kentta[10][10], int ammukset, int ennatys)
/*/Tarkistaa onko ylös-alas-menonappuloita B1 tai B5 painettu 
ja liikuttaa kursoria ylös tai alas ja tulostaa tarvittaessa uudestaan laudan*/
{
			if (!(PINA & (1 << PA4)))
				{
				y = y+1;
				if(y>9) {
					y = 0;
					}
				nayton_tulostus(y, kentta, ammukset, ennatys);
				sei();
				_delay_ms(150);
				cli();
				_delay_ms(50);
				}
			if (!(PINA & (1 << PA0)))
				{
				y = y-1;
				if(y<0)
					y = 9;
				nayton_tulostus(y, kentta, ammukset, ennatys);
				sei();
				_delay_ms(150);
				cli();
				_delay_ms(50);
				}
			return y;
}

int voitontarkistus(char kentta[10][10]) {
/*/Lasketaan pelilaudalta löydetyt laivat*/
    int i = 0, j = 0, tuhotut = 0;
	for (i=0; i<=9; i++)
		{ 
		for (j=0; j<10; j++) {
			if(kentta[i][j] == '*')
				tuhotut++;
		}	
		}
	return tuhotut;
}

int pelinjuoksutus(char kentta[10][10], char aikentta[10][10], int ennatys)
{
/*/Pyörii kun peli on käynnissä, pelin loppuessa kertoo voitettiinko vai hävittiinkö*/
	int x = 1, y = 0, z = 0, ammukset = 99, tuhotut = 0;
	nayton_tulostus(y, kentta, ammukset, ennatys);
	while (1) {
			lcd_gotoxy(x,y);
			x = tarkista_nappulat_x(x);
			y = tarkista_nappulat_y(y, kentta, ammukset, ennatys);
						

			if (!(PINA & (1 << PA2))) {
	        	PORTA |= (1 << PA6);
				if (z == 0 && kentta[y][x-1] != ' ' && aikentta[y][x-1] != '*') {
					ammukset--;
					z = 1;								/*/z estää painalluksen rekisteröitymisen moneen kertaan*/
					}
				if (ammukset == 0){
					return 2;
					}
				if (aikentta[y][x-1] == '*')
					kentta[y][x-1] = '*';
				else
					kentta[y][x-1] = ' ';
				
				nayton_tulostus(y, kentta, ammukset, ennatys);
				_delay_ms(200);
				}
				
			else {
				PORTA &= ~(1 << PA6);
				cli();
				if (z == 1) {
					z = 0;
					}
				}

			
			lcd_write_ctrl(LCD_ON | LCD_BLINK);

			tuhotut = voitontarkistus(kentta);
			if (tuhotut == 18) {
				if (ammukset > ennatys)
					EEPROM_write(0, ammukset);
				return 1;
				}
	}
}

int laivojen_tarkistus(char aikentta[10][10], int x, int y)
/*/Lasketaan koordinaatteja ympäröivissä koordinaateissa olevat laivat*/
{
	int lukumaara = 0, i = 0, j = 0;
	if ((x > 0) && (x < 9) && (y > 0) && (y < 9)) {
	 	for (i=-1; i<2; i++){
	 		for(j=-1; j<2; j++){
				if (aikentta[y+i][x+j] == '*')
					lukumaara++;
			}
		}
	}
	if (x == 0) {
		if (y == 0) {
			if ((aikentta[y+1][x] == '*') || (aikentta[y+1][x+1] == '*') || (aikentta[y][x+1] == '*')) {
				lukumaara++;
			}
		}
		if (y == 9) {
			if ((aikentta[y-1][x] == '*') || (aikentta[y-1][x+1] == '*') || (aikentta[y][x+1] == '*')) {
				lukumaara++;
			}
		}
		else {
			for (i=-1; i<2; i++){
	 			for(j=0; j<2; j++){
					if (aikentta[y+i][x+j] == '*')
						lukumaara++;
				}
			}	
		}
	}
	if (x == 9) {
		if (y == 0) {
			if ((aikentta[y+1][x] == '*') || (aikentta[y+1][x-1] == '*') || (aikentta[y][x-1] == '*')) {
				lukumaara++;
			}
		}
		if (y == 9) {
			if ((aikentta[y-1][x] == '*') || (aikentta[y-1][x-1] == '*') || (aikentta[y][x-1] == '*')) {
				lukumaara++;
			}
		}
		else {
			for (i=-1; i<2; i++){
	 			for(j=0; j<2; j++){
					if (aikentta[y+i][x-j] == '*')
						lukumaara++;
				}
			}	
		}
	}
	if (y == 0) {
		for (i=0; i<2; i++){
	 			for(j=-1; j<2; j++){
					if (aikentta[y+i][x+j] == '*')
						lukumaara++;
				}
			}	
	}
	if (y == 9) {
		for (i=0; i<2; i++){
	 			for(j=-1; j<2; j++){
					if (aikentta[y-i][x+j] == '*')
						lukumaara++;
				}
			}	
	}
	return lukumaara;
}

void arvo_laivat(char aikentta[10][10]) 
{
/*/Arvotaan laivat (5, 4, 2*3, 2, 1) tietokoneen laudalle*/
	int randomx = 0, randomy = 0, k = 0, suunta = 0, i = 0, lukumaara = 0, x = 0, y = 0;
	while(1) {
		randomx = rand() % 10;
		randomy = rand() % 10;
		if (aikentta[randomy][randomx] != '*') {
			suunta = rand() % 2;
			if (suunta == 0 && (randomy + 4 <= 9)) {
				aikentta[randomy][randomx] = '*';
				aikentta[randomy + 1][randomx] = '*';
				aikentta[randomy + 2][randomx] = '*';
				aikentta[randomy + 3][randomx] = '*';
				aikentta[randomy + 4][randomx] = '*';
				break;

			}
			if (suunta == 1 && (randomx + 4 <= 9)) {
				aikentta[randomy][randomx] = '*';
				aikentta[randomy][randomx + 1] = '*';
				aikentta[randomy][randomx + 2] = '*';
				aikentta[randomy][randomx + 3] = '*';
				aikentta[randomy][randomx + 4] = '*';
				break;
			}
		}
	}
	while(1) {
		lukumaara = 0;
		randomx = rand() % 10;
		randomy = rand() % 10;
		if (aikentta[randomy][randomx] != '*') {
			suunta = rand() % 2;
			if (suunta == 0 && (randomy + 3 <= 9)) {
				for (i=0; i<4; i++) {
					y = randomy + i;
					lukumaara = lukumaara + laivojen_tarkistus(aikentta, randomx, y);
				}
				if (lukumaara == 0) {
					aikentta[randomy][randomx] = '*';
					aikentta[randomy + 1][randomx] = '*';
					aikentta[randomy + 2][randomx] = '*';
					aikentta[randomy + 3][randomx] = '*';
					break;
				}
			}
			if (suunta == 1 && (randomx + 3 <= 9)) {
				for (i=0; i<4; i++) {
					x = randomx + i;
					lukumaara = lukumaara + laivojen_tarkistus(aikentta, x, randomy);
				}
				if (lukumaara == 0) {
					aikentta[randomy][randomx] = '*';
					aikentta[randomy][randomx + 1] = '*';
					aikentta[randomy][randomx + 2] = '*';
					aikentta[randomy][randomx + 3] = '*';
					break;
				}
			}
		}
	}
	while(k<2) {
		lukumaara = 0;
		randomx = rand() % 10;
		randomy = rand() % 10;
		if (aikentta[randomy][randomx] != '*') {
			suunta = rand() % 2;
			if (suunta == 0 && (randomy + 2 <= 9)) {
				for (i=0; i<3; i++) {
					y = randomy + i;
					lukumaara = lukumaara + laivojen_tarkistus(aikentta, randomx, y);
				}
				if (lukumaara == 0) {
					aikentta[randomy][randomx] = '*';
					aikentta[randomy + 1][randomx] = '*';
					aikentta[randomy + 2][randomx] = '*';
					k++;
				}
			}
			if (suunta == 1 && (randomx + 2 <= 9)) {
				for (i=0; i<3; i++) {
					x = randomx + i;
					lukumaara = lukumaara + laivojen_tarkistus(aikentta, x, randomy);
				}
				if (lukumaara == 0) {
					aikentta[randomy][randomx] = '*';
					aikentta[randomy][randomx + 1] = '*';
					aikentta[randomy][randomx + 2] = '*';
					k++;
				}
			}
		}
	}
	while(1) {
		lukumaara = 0;
		randomx = rand() % 10;
		randomy = rand() % 10;
		if (aikentta[randomy][randomx] != '*') {
			suunta = rand() % 2;
			if (suunta == 0 && (randomy + 2 <= 9)) {
				for (i=0; i<2; i++) {
					y = randomy + i;
					lukumaara = lukumaara + laivojen_tarkistus(aikentta, randomx, y);
				}
				if (lukumaara == 0) {
					aikentta[randomy][randomx] = '*';
					aikentta[randomy + 1][randomx] = '*';
					break;
				}
			}
			if (suunta == 1 && (randomx + 2 <= 9)) {
				for (i=0; i<2; i++) {
					x = randomx + i;
					lukumaara = lukumaara + laivojen_tarkistus(aikentta, x, randomy);
				}
				if (lukumaara == 0) {
					aikentta[randomy][randomx] = '*';
					aikentta[randomy][randomx + 1] = '*';
					break;
				}
			}
		}
	}
	while(1) {
		lukumaara = 0;
		randomx = rand() % 10;
		randomy = rand() % 10;
		if (aikentta[randomy][randomx] != '*') {
			lukumaara = lukumaara + laivojen_tarkistus(aikentta, randomx, randomy);
			if (lukumaara == 0) {
				aikentta[randomy][randomx] = '*';
				break;
			}
		}
	}
}

void tuloksenilmoitus(int tulos) {
/*/Tulostetaan näytölle pelin tulos ja että uusi peli alkaa nappia painamalla*/
	lcd_write_ctrl(LCD_CLEAR);
	lcd_gotoxy(0,0);
	if(tulos == 1) {
	lcd_write_data('V');
	lcd_write_data('O');
	lcd_write_data('I');
	lcd_write_data('T');
	lcd_write_data('I');
	lcd_write_data('T');
	}
	if(tulos == 2) {
	lcd_write_data('H');
	lcd_write_data('A');
	lcd_write_data('V');
	lcd_write_data('I');
	lcd_write_data('S');
	lcd_write_data('I');
	lcd_write_data('T');
	}
	lcd_write_data(',');
	lcd_write_data(' ');
	lcd_write_data('U');
	lcd_write_data('U');
	lcd_write_data('S');
	lcd_write_data('I');
	lcd_gotoxy(0,1);
	lcd_write_data('P');
	lcd_write_data('E');
	lcd_write_data('L');
	lcd_write_data('I');
	lcd_write_data(' ');
	lcd_write_data('N');
	lcd_write_data('A');
	lcd_write_data('P');
	lcd_write_data('I');
	lcd_write_data('S');
	lcd_write_data('T');
	lcd_write_data('A');
	_delay_ms(500);
	while(1) {
		if (!(PINA & (1 << PA2))){
			_delay_ms(10);
			if ((PINA & (1 << PA2))) {				
				lcd_write_ctrl(LCD_CLEAR);
				break;
			}
		}
	}
}

void alkuanimaatio() {
/*/Tulostetaan alkuanimaatio ja soitetaan musiikki: Laiva liikkuu ylärivillä
ja alarivillä aallot, lopuksi teksti "LAIVANUPOTUS"

John Williamsin Imperial Marchin sävelet hertseinä ja nuottien kestot lähde: http://air.imag.fr/images/1/1b/ImperialMarch.pde.txt*/
	int i = 0;

	sei();						/*/Sallitaan keskeytykset (ääni päälle), tulostetaan laiva ja aallokko*/
	lcd_write_data('0'-1);
	alkuanimaation_alarivi(1);
	OCR1AL = 0x23;				/*/Muutetaan äänen taajuutta rekisterissä ja soitetaan ääntä 500ms*/
	_delay_ms(500);
	cli();						/*/Kielletään keskeytykset (ääni pois päältä), pidetään taukoa 50ms*/
	_delay_ms(50);

	sei();
	lcd_write_data('_');
	lcd_write_data('0'-1);
	alkuanimaation_alarivi(0);
	_delay_ms(500);
	cli();
	_delay_ms(50);

	sei();
	lcd_write_data('_');
	lcd_write_data('_');
	lcd_write_data('0'-1);
	alkuanimaation_alarivi(1);
	_delay_ms(500);
	cli();
	_delay_ms(50);

	sei();
	lcd_write_data('L');
	lcd_write_data('_');
	lcd_write_data('_');
	lcd_write_data('0'-1);
	alkuanimaation_alarivi(0);
	OCR1AL = 0x2c;
	_delay_ms(350);
	cli();
	_delay_ms(50);

	sei();
	for (i=1; i<7; i++) {
		lcd_write_ctrl(LCD_CLEAR);
		lcd_gotoxy(i,0);
		lcd_write_data('L');
		lcd_write_data('_');
		lcd_write_data('_');
		lcd_write_data('0'-1);
		alkuanimaation_alarivi(i);
		if (i==1 || i==4) {
			OCR1AL = 0x1e;
			_delay_ms(150);
			}
		if (i==2) {
			OCR1AL = 0x23;
			_delay_ms(500);
			}
		if (i==3) {
			OCR1AL = 0x2c;
			_delay_ms(350);
			}
		if (i==5) {
			OCR1AL = 0x23;
			_delay_ms(600);
			}
		if (i==6) {
			OCR1AL = 0x18;
			_delay_ms(500);
			}
		cli();
		_delay_ms(50);
		sei();
		}

	lcd_gotoxy(2,1);

	lcd_write_data('L');	/*/Tulostetaan kirjain ja soitetaan sävel*/
	OCR1AL = 0x18;
	_delay_ms(500);
	cli();
	_delay_ms(50);
	sei();

	lcd_write_data('A');
	OCR1AL = 0x18;
	_delay_ms(500);
	cli();
	_delay_ms(50);
	sei();

	lcd_write_data('I');
	OCR1AL = 0x16;
	_delay_ms(350);
	cli();
	_delay_ms(50);
	sei();
	OCR1AL = 0x1e;
	_delay_ms(150);
	cli();
	_delay_ms(50);

	lcd_write_data('V');
	OCR1AL = 0x25;
	sei();
	_delay_ms(500);
	cli();
	_delay_ms(50);

	lcd_write_data('A');
	OCR1AL = 0x2c;
	sei();
	_delay_ms(350);
	cli();
	_delay_ms(50);
	OCR1AL = 0x1e;
	sei();
	_delay_ms(150);
	cli();
	_delay_ms(50);

	lcd_write_data('N');
	OCR1AL = 0x23;
	sei();
	_delay_ms(500);
	cli();
	_delay_ms(50);

	lcd_write_data('U');
	OCR1AL = 0x12;
	sei();
	_delay_ms(500);
	cli();
	_delay_ms(50);

	lcd_write_data('P');
	OCR1AL = 0x23;
	sei();
	_delay_ms(300);
	cli();
	_delay_ms(50);
	sei();
	_delay_ms(150);
	cli();
	_delay_ms(50);

	lcd_write_data('O');
	OCR1AL = 0x12;
	sei();
	_delay_ms(500);
	cli();
	_delay_ms(50);

	lcd_write_data('T');
	OCR1AL = 0x13;
	sei();
	_delay_ms(250);
	cli();
	_delay_ms(50);

	lcd_write_data('U');
	OCR1AL = 0x14;
	sei();
	_delay_ms(250);
	cli();
	_delay_ms(50);

	lcd_write_data('S');
	OCR1AL = 0x15;
	sei();
	_delay_ms(125);
	cli();
	_delay_ms(25);

	OCR1AL = 0x16;
	sei();
	_delay_ms(125);
	cli();
	_delay_ms(25);

	OCR1AL = 0x15;
	sei();
	_delay_ms(150);
	cli();
	OCR1AL = 0x3e;

	while(1) {								/*/Peli alkaa kun keskinappia on painettu*/
		if (!(PINA & (1 << PA2))){
			_delay_ms(10);
			if ((PINA & (1 << PA2))) {				
				srand(TCNT1);
				lcd_write_ctrl(LCD_CLEAR);
				break;
			}
		}
	}

}

void alkuanimaation_alarivi(int a) {
/*/Aaltojen tulostus näytön alariville alkuanimaation aikana*/
	lcd_gotoxy(0,1);
	int i = 0;
	if ((a%2) == 0) {
		for (i=0; i<8; i++) {
			lcd_write_data('^');
			lcd_write_data('v');
		}
	}
	else {
		for (i=0; i<8; i++) {
			lcd_write_data('v');
			lcd_write_data('^');
		}
	}
	lcd_gotoxy(0,0);
}	

void EEPROM_write(unsigned int uiAddress, unsigned int ucData)
/*/EEPROM-muistiin kirjoitus, käytetään ennätyksien muistamisessa. Lähde datasheet sivu 23 http://www.atmel.com/Images/doc2467.pdf*/
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEWE));
	/* Set up address and data registers */
	EEAR = uiAddress;
	EEDR = ucData;
	/* Write logical one to EEMWE */
	EECR |= (1<<EEMWE);
	/* Start eeprom write by setting EEWE */
	EECR |= (1<<EEWE);
}

unsigned char EEPROM_read(unsigned int uiAddress)
/*/EEPROM-muistista luku, käytetään ennätyksien muistamisessa. Lähde datasheet sivu 23 http://www.atmel.com/Images/doc2467.pdf*/
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEWE));
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from data register */
	return EEDR;
}

int main(void) 
{
	init();
	char kentta[10][10];
	char aikentta[10][10];
	int ennatysromista = EEPROM_read(0);
	if (ennatysromista > 99) /*/Tarkistetaan ettei EEPROM-muistissa ole jotain muuta kuin ennätys.*/
		EEPROM_write(0, 0); /*/Jos on niin nollataan ennätyksiä varten*/

	alkuanimaatio();
	while(1) {
		int tulos = 0;
		int ennatys = EEPROM_read(0);
		kentan_taytto(kentta);
		aikentan_taytto(aikentta);
		arvo_laivat(aikentta);	
		tulos = pelinjuoksutus(kentta, aikentta, ennatys);
		tuloksenilmoitus(tulos);
	}
}

ISR(TIMER1_COMPA_vect) {

	/* vaihdetaan kaiutin pinnien tilat XOR operaatiolla */
 	PORTE ^= (1 << PE4) | (1 << PE5); 
}

