/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 



#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"
#define PI 3.14159265

struct ili9488_opt_t g_ili9488_display_opt;
/************************************************************************/
/* variaveis globais                                                    */

int quantidades_de_rotacoes=-1;
int hora,minutos,segundos;
int velocidade,velocidade_angular;
float distancia;
double raio=0.325;
char buffer_rotacoes[32];
char buffer_tempo_total[32];
char buffer_distancia[32];


volatile Bool f_rtt_alarme = false;
/************************************************************************/
/* prototypes                                                           */

void pin_toggle(Pio *pio, uint32_t mask);
void io_init(void);
static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);
/************************************************************************/
/* defines                                                              */
/************************************************************************/
// Bot?o
#define BUT1_PIO      PIOB
#define BUT1_PIO_ID   ID_PIOB
#define BUT1_IDX  1
#define BUT1_IDX_MASK (1 << BUT1_IDX)

#define LED_PIO       PIOC
#define LED_PIO_ID    ID_PIOC
#define LED_IDX       8u
#define LED_IDX_MASK  (1u << LED_IDX)

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

void but_callback(void)
{
	quantidades_de_rotacoes+=1;
	
}
void io_init(void)
{
	// Inicializa clock do perif?rico PIO responsavel pelo botao
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);
	// Configura PIO para lidar com o pino do bot?o como entrada
	// com pull-up
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_IDX_MASK, PIO_PULLUP|PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT1_PIO,1,20);
	
	pio_handler_set(BUT1_PIO,
	BUT1_PIO_ID,
	BUT1_IDX_MASK,
	PIO_IT_FALL_EDGE,
	but_callback);
		// Ativa interrup??o
	pio_enable_interrupt(BUT1_PIO, BUT1_IDX_MASK);
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 3); // Prioridade 3
}
void pin_toggle(Pio *pio, uint32_t mask){
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}


void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}	
}
static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}
/************************************************************************/
/* interrupcoes                                                         */
/************************************************************************/

void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		pin_toggle(LED_PIO, LED_IDX_MASK);    // BLINK Led
		f_rtt_alarme = true;                  // flag RTT alarme
	}
	if(segundos==60){
		segundos=0;
		minutos+=1;
	}else{segundos+=1;}
	distancia=2*PI*quantidades_de_rotacoes*raio;
}

	/* Initialize TC */

int main(void) {
	board_init();
	sysclk_init();	
	configure_lcd();
	// Desliga watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;
	io_init();
	
	// Inicializa RTT com IRQ no alarme.
	f_rtt_alarme = true;
	
	


	while(1) {
		if (f_rtt_alarme){
      uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
      uint32_t irqRTTvalue  = 2;
      
      // reinicia RTT para gerar um novo IRQ
      RTT_init(pllPreScale, irqRTTvalue);         
      sprintf(buffer_rotacoes,"%d",quantidades_de_rotacoes);
	  sprintf(buffer_tempo_total,"00:%d:%d",minutos,segundos);
	  sprintf(buffer_distancia,"m : %lf",distancia);
	  font_draw_text(&calibri_36, buffer_distancia, 50, 50, 1);
      font_draw_text(&calibri_36, buffer_tempo_total, 50, 100, 1);
      font_draw_text(&calibri_36, buffer_rotacoes, 50, 200, 2);
     /*
      * caso queira ler o valor atual do RTT, basta usar a funcao
      *   rtt_read_timer_value()
      */
      
      f_rtt_alarme = false;
		
		}
	}
}