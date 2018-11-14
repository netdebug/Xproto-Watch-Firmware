#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include "main.h"
#include "utils.h"
#include "games.h"

void InitSnake(void);
void NewFruit(void);
void MoveSnake(SnakeStruct *Player);
void SnakeEngine(void);

void InitSnake(void) {
    uint16_t k=0;
    for(uint8_t i=0; i<32; i++) {
        for(uint8_t j=0; j<32; j++) {
            Temp.SNAKE.board[i][j]=0;
            Temp.SNAKE.Player1.x[k]=0;
            Temp.SNAKE.Player1.y[k]=0;
            Temp.SNAKE.Player2.x[k]=0;
            Temp.SNAKE.Player2.y[k]=0;
            k++;
        }
    }
    Temp.SNAKE.Player1.x[0]=Temp.SNAKE.Player1.x[1]=2;
    Temp.SNAKE.Player1.y[0]=Temp.SNAKE.Player1.y[1]=2;
    Temp.SNAKE.Player1.size=2;
    Temp.SNAKE.Player1.direction = GO_RIGHT;
    Temp.SNAKE.Player2.x[0]=Temp.SNAKE.Player2.x[1]=29;
    Temp.SNAKE.Player2.y[0]=Temp.SNAKE.Player2.y[1]=29;
    Temp.SNAKE.Player2.size=2;
    Temp.SNAKE.Player2.direction = GO_LEFT;
}

// Spawn new fruit
void NewFruit(void) {
    // Linear Pixel Shuffling for Image Processing
    #define G1 41
    #define G2 60
    #define Sincrement (G2 - G1)
    do {    /* determine next pixel */
        Temp.SNAKE.Fruitx = ( Temp.SNAKE.Fruitx + Sincrement ) % G1;
        Temp.SNAKE.Fruity = ( Temp.SNAKE.Fruity + Sincrement ) % G2;
    } while ((Temp.SNAKE.Fruitx >=32) || (Temp.SNAKE.Fruity >= 32) ||   // Out of bounds
      (Temp.SNAKE.board[Temp.SNAKE.Fruitx][Temp.SNAKE.Fruity]!=EMPTY)); // Occupied
    Temp.SNAKE.board[Temp.SNAKE.Fruitx][Temp.SNAKE.Fruity] = FRUIT;       // New head position
}

void Snake(void) {
    uint8_t exit=0;
    Temp.SNAKE.Player1.state = HUMAN_PLAYER1;
    Temp.SNAKE.Player2.state = NO_PLAYER2;
    setbit(MStatus,update);
    do {
        if(testbit(MStatus, update)) {
            clrbit(MStatus, update);
            if(testbit(Misc,userinput)) {
                clrbit(Misc, userinput);
                if(testbit(Buttons, K1)) Temp.SNAKE.Player1.state++; if(Temp.SNAKE.Player1.state>CPU_PLAYER1) Temp.SNAKE.Player1.state=NO_PLAYER1;
                if(testbit(Buttons, K2)) Temp.SNAKE.Player2.state++; if(Temp.SNAKE.Player2.state>CPU_PLAYER2) Temp.SNAKE.Player2.state=NO_PLAYER2;
                if(testbit(Buttons, K3)) SnakeEngine();
                if(testbit(Buttons, KML)) exit=1;
            }
            clr_display_1();
            lcd_goto(45,0); lcd_put5x8(PSTR("Snake"));
            lcd_goto(0,2); lcd_put5x8(PSTR("Player 1: "));
            if(Temp.SNAKE.Player1.state==NO_PLAYER1) lcd_put5x8(PSTR("None"));
            else if(Temp.SNAKE.Player1.state==HUMAN_PLAYER1) lcd_put5x8(PSTR("Human"));
            if(Temp.SNAKE.Player1.state==CPU_PLAYER1) lcd_put5x8(PSTR("CPU"));
            lcd_goto(0,3); lcd_put5x8(PSTR("Player 2: "));
            if(Temp.SNAKE.Player2.state==NO_PLAYER2) lcd_put5x8(PSTR("None"));
            else if(Temp.SNAKE.Player2.state==HUMAN_PLAYER2) lcd_put5x8(PSTR("Human"));
            if(Temp.SNAKE.Player2.state==CPU_PLAYER2) lcd_put5x8(PSTR("CPU"));
            lcd_goto(0,15); lcd_put5x8(PSTR("Player1 Player2 Start"));
        }
        dma_display();
        WaitDisplay();
        SLEEP.CTRL = SLEEP_SMODE_PSAVE_gc | SLEEP_SEN_bm;
        asm("sleep");
        asm("nop");
    } while(!exit);
}

// Arrays to select only one direction, if more than one direction was selected
// Select rightmost bit
const uint8_t Direction1[16] PROGMEM = {
    GO_UP,          // 0000
    GO_UP,          // 0001
    GO_DOWN,        // 0010
    GO_UP,          // 0011
    GO_LEFT,        // 0100
    GO_UP,          // 0101
    GO_DOWN,        // 0110
    GO_UP,          // 0111
    GO_RIGHT,       // 1000
    GO_UP,          // 1001
    GO_DOWN,        // 1010
    GO_UP,          // 1011
    GO_LEFT,        // 1100
    GO_UP,          // 1101
    GO_DOWN,        // 1110
    GO_UP,          // 1111
};

// Arrays to select only one direction, if more than one direction was selected
// Select lefttmost bit
const uint8_t Direction2[16] PROGMEM = {
    GO_RIGHT,       // 0000
    GO_UP,          // 0001
    GO_DOWN,        // 0010
    GO_DOWN,        // 0011
    GO_LEFT,        // 0100
    GO_LEFT,        // 0101
    GO_LEFT,        // 0110
    GO_LEFT,        // 0111
    GO_RIGHT,       // 1000
    GO_RIGHT,       // 1001
    GO_RIGHT,       // 1010
    GO_RIGHT,       // 1011
    GO_RIGHT,       // 1100
    GO_RIGHT,       // 1101
    GO_RIGHT,       // 1110
    GO_RIGHT,       // 1111
};

void MoveSnake(SnakeStruct *Player) {
    if(Player->state==CPU_PLAYER1 || Player->state==CPU_PLAYER2) {                          // CPU AI
        uint8_t FruitDirection=0;
        uint8_t GoodDirection=0x0F;
        if(Player->y[0]>Temp.SNAKE.Fruity) FruitDirection|=GO_UP;                           // Find fruit
        if(Player->y[0]<Temp.SNAKE.Fruity) FruitDirection|=GO_DOWN;
        if(Player->x[0]>Temp.SNAKE.Fruitx) FruitDirection|=GO_LEFT;
        if(Player->x[0]<Temp.SNAKE.Fruitx) FruitDirection|=GO_RIGHT;
        if(Temp.SNAKE.board[Player->x[0]][Player->y[0]-1]>FRUIT) GoodDirection&=~GO_UP;     // Avoid obstacles
        if(Temp.SNAKE.board[Player->x[0]][Player->y[0]+1]>FRUIT) GoodDirection&=~GO_DOWN;
        if(Temp.SNAKE.board[Player->x[0]-1][Player->y[0]]>FRUIT) GoodDirection&=~GO_LEFT;
        if(Temp.SNAKE.board[Player->x[0]+1][Player->y[0]]>FRUIT) GoodDirection&=~GO_RIGHT;
        FruitDirection &= GoodDirection;                                                    // Don't follow fruit thru obstacles
        if(FruitDirection) {                                                                // A good path to fruit exists
            if((FruitDirection & Player->direction)==0) {                                   // Snake not following fruit already
                Player->direction=FruitDirection;
            }
        } else Player->direction=GoodDirection;                                             // Otherwise, just avoid walls
        // Make sure only one direction is selected
        if(prandom()&0x01) Player->direction = pgm_read_byte(Direction1+Player->direction);
        else Player->direction = pgm_read_byte(Direction2+Player->direction);
    }
    if(Player->state) { // Snake alive
        Temp.SNAKE.board[Player->x[Player->size-1]][Player->y[Player->size-1]] = 0;             // Erase tail from board
        for(uint16_t i=Player->size-1; i>0; i--) {                                              // Drag body
            Player->x[i]=Player->x[i-1];
            Player->y[i]=Player->y[i-1];
        }
        Temp.SNAKE.board[Player->x[Player->size-1]][Player->y[Player->size-1]] = Player->state;     // New tail position
        switch(Player->direction) {                                         // Move head
            case GO_UP:    Player->y[0]--; break;
            case GO_DOWN:  Player->y[0]++; break;
            case GO_LEFT:  Player->x[0]--; break;
            case GO_RIGHT: Player->x[0]++; break;
        }
        if(Temp.SNAKE.board[Player->x[0]][Player->y[0]]>=OBSTACLE ||        // New position is an obstacle or a body
            Player->x[0]>=32 || Player->y[0]>=32) {                         // Out of boundaries
            Player->state = 0;                                              // Died
            return;
        }            
        Temp.SNAKE.board[Player->x[0]][Player->y[0]] = Player->state;       // New head position
        if(Temp.SNAKE.Fruitx==Player->x[0] && Temp.SNAKE.Fruity==Player->y[0]) {  // Got Fruit!
            if(Player->size<=(32*32)) {
                Player->x[Player->size]=Player->x[Player->size-1];          // Extend tail
                Player->y[Player->size]=Player->y[Player->size-1];
                Player->size++;
                NewFruit();
            }                
        }
    } else {
        if(Player->size>3) {
            Temp.SNAKE.board[Player->x[Player->size-1]][Player->y[Player->size-1]] = 0; // Erase tail from board
            Player->size--;                               // Shrink if dead
        }            
    }        
}

void SnakeBoard(void) {
    uint8_t x,y;
    for(uint8_t i=0; i<32; i++) {
        for(uint8_t j=0; j<32; j++) {
            x=i*4; y=j*4;
            switch(Temp.SNAKE.board[i][j]) {
                case  EMPTY:
                break;
                case FRUIT:
                                      set_pixel(x+1,y  ); set_pixel(x+2,y  ); 
                    set_pixel(x,y+1);                                         set_pixel(x+3,y+1);
                    set_pixel(x,y+2);                                         set_pixel(x+3,y+2);
                                      set_pixel(x+1,y+3); set_pixel(x+2,y+3); 
                break;
                case HUMAN_PLAYER1:
                case CPU_PLAYER1:
                    set_pixel(x,y  ); set_pixel(x+1,y  ); set_pixel(x+2,y  ); set_pixel(x+3,y  );
                    set_pixel(x,y+1);                                         set_pixel(x+3,y+1);
                    set_pixel(x,y+2);                                         set_pixel(x+3,y+2);
                    set_pixel(x,y+3); set_pixel(x+1,y+3); set_pixel(x+2,y+3); set_pixel(x+3,y+3);
                break;
                case HUMAN_PLAYER2:
                case CPU_PLAYER2:
                    set_pixel(x,y  ); set_pixel(x+1,y  ); set_pixel(x+2,y  ); set_pixel(x+3,y  );
                    set_pixel(x,y+1); set_pixel(x+1,y+1); set_pixel(x+2,y+1); set_pixel(x+3,y+1);
                    set_pixel(x,y+2); set_pixel(x+1,y+2); set_pixel(x+2,y+2); set_pixel(x+3,y+2);
                    set_pixel(x,y+3); set_pixel(x+1,y+3); set_pixel(x+2,y+3); set_pixel(x+3,y+3);
                break;
                default:
                                      set_pixel(x+1,y  );                     set_pixel(x+3,y  );
                    set_pixel(x,y+1);                     set_pixel(x+2,y+1); 
                                      set_pixel(x+1,y+2);                     set_pixel(x+3,y+2);
                    set_pixel(x,y+3);                     set_pixel(x+2,y+3);
                break;
            }
        }
    }
}

void SnakeEngine(void) {
    InitSnake();
    NewFruit();
    uint8_t exit=0;
    setbit(MStatus,update);
    do {
        if(testbit(Misc,userinput)) {
            clrbit(Misc, userinput);
            if(testbit(Buttons, KML)) exit=1;
            if(testbit(Buttons, KBL)) {
                if(Temp.SNAKE.Player1.direction==GO_UP) Temp.SNAKE.Player1.direction=GO_LEFT;
                else if(Temp.SNAKE.Player1.direction==GO_DOWN) Temp.SNAKE.Player1.direction=GO_RIGHT;
                else if(Temp.SNAKE.Player1.direction==GO_LEFT) Temp.SNAKE.Player1.direction=GO_DOWN;
                else if(Temp.SNAKE.Player1.direction==GO_RIGHT) Temp.SNAKE.Player1.direction=GO_UP;
            }
            else if(testbit(Buttons, KBR)) {
                if(Temp.SNAKE.Player1.direction==GO_UP) Temp.SNAKE.Player1.direction=GO_RIGHT;
                else if(Temp.SNAKE.Player1.direction==GO_DOWN) Temp.SNAKE.Player1.direction=GO_LEFT;
                else if(Temp.SNAKE.Player1.direction==GO_LEFT) Temp.SNAKE.Player1.direction=GO_UP;
                else if(Temp.SNAKE.Player1.direction==GO_RIGHT) Temp.SNAKE.Player1.direction=GO_DOWN;
            }
        }
        clr_display_1();
        if(Temp.SNAKE.Player1.state!=NO_PLAYER1) MoveSnake(&Temp.SNAKE.Player1);
        if(Temp.SNAKE.Player2.state!=NO_PLAYER2) MoveSnake(&Temp.SNAKE.Player2);
        SnakeBoard();
        dma_display();
        WaitDisplay();
        SLEEP.CTRL = SLEEP_SMODE_PSAVE_gc | SLEEP_SEN_bm;
        asm("sleep");
        asm("nop");
    } while(!exit);
}
