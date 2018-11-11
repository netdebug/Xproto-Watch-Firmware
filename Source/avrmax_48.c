/*****************************************************************************

 Xmegalab / XMultiKit Chess application, ported from:

 ****************************************************************************
 *                               AVR-Max,                                   *
 * A chess program smaller than 2KB (of non-blank source), by H.G. Muller   *
 * AVR ATMega88V port, iterative Negamax, by Andre Adrian                   *
 ************************************************************************** *
 * 14dez2008 adr: merge avr_c_io and uMax                                   *
 * 18dez2008 adr: return -l if Negamax stack underrun                       *
 * 24dez2008 adr: add functions 1=new game, 2=set level, 3=show PV          *
 * 26dez2008 adr: bug fix, undo modifications on uMax 4.8                   *
 * 29dez2009 adr: Key Debouncing with inhibit (Sperrzeit nach Loslassen)    *

   Stufe (Level)
   1 Blitz      = min. 0.5s, typ. 7s
   2 2*Blitz    = min. 1s,   typ. 15s    
   3 4*Blitz    = min. 2s,   typ. 22s
   4 Aktiv/2    = min. 4s,   typ. 
   5 Aktiv      = min. 8s,   typ. 30s
   6 2*Aktiv    = min. 16s,  typ.
   7 Turnier/2  = min. 32s,  typ. 
   8 Turnier    = min. 64s,  typ. 
  FN 2*Turnier  = min. 128s, typ.
  CL 4*Turnier  = min. 256s, typ.                                           */

#include <avr/io.h>       
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include "main.h"
#include "utils.h"

// Chess application variables
typedef union {
    unsigned char c[4];
} TIMER;

volatile TIMER timer;       // wird alle 4.096ms erhoeht
unsigned char st=0;         // Level (Sets thinking time)

void putled(unsigned char ndx, unsigned char c) {
    //lcd_goto(ndx,15);
    //putchar5x8(c);
}

unsigned char getkey(void) {
    unsigned char ch;
  
    while (!Buttons) {
    //    sleep cpu
    }
    // keycode to ASCII
//    ch = pgm_read_byte(&Letter[key-1]);
    Buttons = 0;
    return ch;
}

void clean() {
    putled(0, ' ');putled(1, ' ');putled(2, ' ');putled(3, ' ');
}

void bold() {
//    led[0]&=0x7F;led[1]&=0x7F;led[2]&=0x7F;led[3]&=0x7F;
}

/***************************************************************************/
/*                               micro-Max,                                */
/* A chess program smaller than 2KB (of non-blank source), by H.G. Muller  */
/* AVR port, iterative version, by Andre Adrian                            */
/***************************************************************************/
/* version 4.81 (1953+ characters) features:                               */
/* - recursive negamax search                                              */
/* - all-capture MVV/LVA quiescence search                                 */
/* - (internal) iterative deepening                                        */
/* - best-move-first 'sorting'                                             */
/* - a hash table storing score and best move                              */
/* - futility pruning                                                      */
/* - king safety through magnetic, frozen king in middle-game              */
/* - R=2 null-move pruning                                                 */
/* - keep hash and repetition-draw detection                               */
/* - better defense against passers through gradual promotion              */
/* - extend check evasions in inner nodes                                  */
/* - reduction of all non-Pawn, non-capture moves except hash move (LMR)   */
/* - full FIDE rules (expt under-promotion) and move-legality checking     */

#define M 0x88                              /* Unused bits in valid square */  
#define S 128                               /* Sign bit of char            */
#define I 8000                              /* Infinity score              */

long N; 
signed int Q,                               /* pass updated eval. score    */
K,                    /* input move, origin square or Infinity as flag     */
i,                                          /* temp. evaluation term       */
s,
Dq,Dl,De,                         /* D() arguments */
DD;                               /* D() return value */

const signed char w[] PROGMEM = {
  0,2,2,7,-1,8,12,23};                         /* relative piece values    */

const signed char o[] PROGMEM = {
  -16,-15,-17,0,1,16,0,1,16,15,17,0,14,18,31,33,0,    /* step-vector lists */
  7,-1,11,6,8,3,6,                             /* 1st dir. in o[] per piece*/
  6,3,5,7,4,5,3,6};                            /* initial piece setup      */

unsigned char L,                               /* input move, target square*/
O,                                          /* pass e.p. flag at game level*/
k,                                          /* moving side 8=white 16=black*/
b[129],                                        /* board: half of 16x8+dummy*/
c[5],                                          /* input move ASCII buffer  */     
R,                                          /* captured non pawn material  */
W,                                           /* @ temporary                */
DE,Dz,Dn,                                      /* D() arguments            */
Da,                                            /* D() state                */
hv;                                            /* zeige Hauptvariante      */

unsigned short r = 1;                     /* pseudo random generator seed */

/* better readability of AVR Program memory arrays */
#define o(ndx)	(signed char)pgm_read_byte(o+(ndx))
#define w(ndx)	(signed char)pgm_read_word(w+(ndx))

void D();

void printboard(void) {
    uint8_t c,i=0,color=0,piece,x,y;
    clr_display_1();
    // Draw board
    for(x=0; x<120; x+=15) {
        for(y=0; y<128; y+=16) {
            if(!testbit(color,0)) fillRectangle(x, y, x+14, y+15, 255);
            color++;
        }
        color++;
    }
    x=1; y=0; color=0;
    // Place pieces
    do {
        if(i&8) { i+=7; y+=2; x=1; color++; }
        else {
      //putchar5x8(".?pnkbrq?P?NKBRQ"[b[i]&15]);
            piece=("@@GHIJKL@A@BCDEF"[b[i]&15])-'@';
            if(piece) {
                c=testbit(color,0);
                if(piece>6) {
                    piece-=6;
                    if(testbit(color,0)) { piece+=6; }
                }
                else {
                    c+=2;
                    if(!testbit(color,0)) { piece+=6; }
                }                
                bitmap_safe(x,y,(uint8_t *)pgm_read_word(Chessbmps+piece),c);
            }                
            x+=15;
            color++;
        }
    } while(++i<128);
    dma_display();
}

void CHESS(void) {
    Temp.GAMES.MP=Temp.GAMES.SA+U;  // Initialize engine stack pointer
    // Timer TCD1: 244.14063Hz
    //PR.PRPD  = 0b11101101;  // Power Reduction: TWI,       , USART1, SPI, HIRES, , TC0
    //TCD1.CTRLA = 0x05;      // Prescaler: clk/64
    //TCD1.PER   = 2047;      // 244.14063Hz
    //TCD1.INTCTRLA = 0x02;   // Timer overflow is a medium level interrupt
    Buttons=0;
    for(;;) { 
A:
        k=16;O=Q=R=0;
        for(W=0;W<sizeof b;++W)b[W]=0;
        W=8;
        while(W--) {
            b[W]=(b[W+112]=o(W+24)+8)+8;b[W+16]=18;b[W+96]=9; /* initial board setup*/
            L=8;while(L--)
            b[16*L+W+8]=(W-4)*(W-4)+(L+L-7)*(L+L-7)/4;       /* center-pts table   */
        }                                                  /*(in unused half b[])*/

        do {                                                /* play loop          */
            printboard();
            W = 4;
            if(Buttons) return;
            else {
                for(;;) {
                    switch (c[W] = '#'/*getkey()*/) {
                        case '9':
                            putled(0, 'F');putled(1, 'N');putled(2, ' ');putled(3, ' ');
                            switch (L = getkey()) {
                                case '1': goto A; break;      // Neues Spiel (new game)
                                case '2':                     // Spielstufe (set level)
                                    for(;;) {
                                        putled(0, 'S');putled(1, 'T');putled(3, st+'1');
                                        if ('#' == (L = getkey())) break;
                                        st=L-'1';
                                    }
                                break;
                                case '3':                     // edit variant
                                    for(;;) {
                                        putled(0, 'H');putled(1, 'V');putled(3, hv+'0');
                                        if ('#' == (L = getkey())) break;
                                        hv=L&1;
                                    }
                                break;
                            }
                        // no break
                        case '*': if(W==0) {
                                    TCD1.INTCTRLA = 0;  // disable timer interrupt
                                    return;
                                 }
                                 else W=5; break;
                        case '#': goto B; break;
                    }
                    if (!(W&1)) {          // true when W is (0, 2, 4)
                        c[W] += 'a' - '1';
                    }
                    switch (W) {
                        default:
    	                    putled(W, c[W]);
    	                    ++W;
                        break;
                        case 4:                 // getkey()
    	                    clean();
	                        c[0]=c[4];
	                        putled(0, c[0]);
	                        W=1;
                        break;
                        case 5:                 // getkey()
    	                    clean();
                            W=0;
                        break;
                    }
                }
            }
B:
            printboard();

            if(c[0]!='#')K=*c-16*c[1]+799,L=c[2]-16*c[3]+799;     /* parse entered move */
            else {
                K=I;
                cli();                  // Interrupts sperren
                N = 128<<st;            // set time control (thinking time)
                N += qrandom();         // Add some randomness to time
                sei();                   // Interrupts erlauben
                clean();
                if (hv) { bold(); }
            }
            Dq=-I;Dl=I;De=Q;DE=O;Dz=1;Dn=3;               /* store arguments of D() */
            Da=0;                                         /* state */
            D();                                          /* accept human move */
            if(*c-'#') {          // Input available
                if(I==DD) {         // input valid
                    bold();
                }
                else {            // .. ungueltig
                    putled(0, 'N');putled(1, 'o');putled(2, 't');putled(3, 'V');
                } 	
                *c='#';
            }
            else {             // Computer move
                putled(0,'a'+(K&7));putled(1,'8'-(K>>4));  
                putled(2,'a'+(L&7));putled(3,'8'-(L>>4&7)); 
            }   
        } while(DD>-I+1);
        putled(0, 'M');putled(1, 'A'); putled(2, 'T');putled(3, 'T');
        getkey();
    }
}

/* better readability of working struct variables */
#define _ Temp.GAMES._
#define q _.q
#define l _.l
#define e _.e
#define E _.E
#define z _.z
#define n _.n
#define m _.m
#define v _.v
#define V _.V
#define P _.P
#define r _.r
#define j _.j
#define B _.B
#define d _.d
#define h _.h
#define C _.C
#define u _.u
#define p _.p
#define x _.x
#define y _.y
#define F _.F
#define G _.G
#define H _.H
#define t _.t
#define X _.X
#define Y _.Y
#define a _.a
#define SA Temp.GAMES.SA
#define MP Temp.GAMES.MP

void D()                                       /* iterative Negamax search */
{                       
D:if (--MP<SA) {               /* stack pointer decrement and underrun check */
  ++MP;DD=-l;goto R;                                    /* simulated return */
 } 
 q=Dq;l=Dl;e=De;E=DE;z=Dz;n=Dn;                          /* load arguments */
 a=Da;                                         /* load return address state*/

 --q;                                          /* adj. window: delay bonus */
 k^=24;                                        /* change sides             */
 d=Y=0;                                        /* start iter. from scratch */
 X=0;//myrand()&~M;                                /* start at random field    */

 while(d++<n||d<3||                            /* iterative deepening loop */
   z&K==I&&(N>=0&d<98||                        /* root: deepen upto time   */
   (K=X,L=Y&~M,d=3)))                          /* time's up: go do best    */
 {x=B=X;                                       /* start scan at prev. best */
  h=Y&S;                                       /* request try noncastl. 1st*/
  if(d<3)P=I;else 
  {*MP=_;Dq=-l;Dl=1-l;De=-e;DE=S;Dz=0;Dn=d-3;   /* save locals, arguments   */
   Da=0;goto D;                                /* Search null move         */
R0:_=*MP;P=DD;                                  /* load locals, return value*/
  }
  m=-P<l|R>35?d>2?-I:e:-P;                     /* Prune or stand-pat       */
  --N;                                         /* node count (for timing)  */
  do{u=b[x];                                   /* scan board looking for   */
   if(u&k)                                     /*  own piece (inefficient!)*/
   {r=p=u&7;                                   /* p = piece type (set r>0) */
    j=o(p+16);                                 /* first step vector f.piece*/
    while(r=p>2&r<0?-r:-o(++j))                /* loop over directions o[] */
    {A:                                        /* resume normal after best */
     y=x;F=G=S;                                /* (x,y)=move, (F,G)=castl.R*/
     do{                                       /* y traverses ray, or:     */
      H=y=h?Y^h:y+r;                           /* sneak in prev. best move */
      if(y&M)break;                            /* board edge hit           */
      m=E-S&b[E]&&y-E<2&E-y<2?I:m;             /* bad castling             */
      if(p<3&y==E)H^=16;                       /* shift capt.sqr. H if e.p.*/
      t=b[H];if(t&k|p<3&!(y-x&7)-!t)break;     /* capt. own, bad pawn mode */
      i=37*w(t&7)+(t&192);                     /* value of capt. piece t   */
      m=i<0?I:m;                               /* K capture                */
      if(m>=l&d>1)goto J;                      /* abort on fail high       */

      v=d-1?e:i-p;                             /* MVV/LVA scoring          */
      if(d-!t>1)                               /* remaining depth          */
      {v=p-4|R>29                              /* early or mid-game K move */
        ?(p-7|R>7                            /* @ early Q move             */
         ?(p<6                                 /* crawling pieces          */
          ?b[x+8]-b[y+8]                       /* center positional pts.   */
          :0)
         :-12)                               /* @ penalize early Q move    */
        :-9;                                   /* penalize mid-game K move */
       b[G]=b[H]=b[x]=0;b[y]=u|32;             /* do move, set non-virgin  */
       if(!(G&M))b[F]=k+6,v+=30;               /* castling: put R & score  */
       if(p<3)                                 /* pawns:                   */
       {v-=9*((x-2&M||b[x-2]-u)+               /* structure, undefended    */
              (x+2&M||b[x+2]-u)-1              /*        squares plus bias */
             +(b[x^16]==k+36))                 /* kling to non-virgin King */
             -(R>>2);                          /* end-game Pawn-push bonus */
        V=y+r+1&S?647-p:2*(u&y+16&32);         /* promotion or 6/7th bonus */
        b[y]+=V;i+=V;                          /* change piece, add score  */
       }
       v+=e+i;V=m>q?m:q;                       /* new eval and alpha       */
       C=d-1-(d>5&p>2&!t&!h);
       C=R>29|d<3|P-I?C:d;                     /* extend 1 ply if in check */
       do
        if(C>2|v>V)
        {*MP=_;Dq=-l;Dl=-V;De=-v;DE=F;Dz=0;Dn=C; /* save locals, arguments  */
         Da=1;goto D;                          /* iterative eval. of reply */
R1:      _=*MP;s=-DD;                           /* load locals, return value*/
      ONGRN();                                  // Quick green blink
        }else s=v;                             /* or fail low if futile    */
       while(s>q&++C<d);v=s;
       if(z&&K-I&&v+I&&x==K&y==L)              /* move pending & in root:  */
       {Q=-e-i;O=F;                            /*   exit if legal & found  */
        R+=i>>7;++MP;DD=l;goto R;               /* captured non-P material  */
       }
       b[G]=k+6;b[F]=b[y]=0;b[x]=u;b[H]=t;     /* undo move,G can be dummy */
      }
      if(v>m)                                  /* new best, update max,best*/
       m=v,X=x,Y=y|S&F;                        /* mark double move with S  */
      if(h){h=0;goto A;}                       /* redo after doing old best*/
      if(x+r-y|u&32|                           /* not 1st step,moved before*/
         p>2&(p-4|j-7||                        /* no P & no lateral K move,*/
         b[G=x+3^r>>1&7]-k-6                   /* no virgin R in corner G, */
         ||b[G^1]|b[G^2])                      /* no 2 empty sq. next to R */
        )t+=p<5;                               /* fake capt. for nonsliding*/
      else F=y;                                /* enable e.p.              */
     OFFGRN();
     }while(!t);                               /* if not capt. continue ray*/
  }}}while((x=x+9&~M)-B);                      /* next sqr. of board, wrap */
J:if(m>I-M|m<M-I)d=98;                         /* mate holds to any depth  */
  m=m+I|P==I?m:0;                              /* best loses K: (stale)mate*/
#ifndef __unix__  
   if(z&hv&d>2)                                   
   {putled(0,'a'+(X&7));putled(1,'8'-(X>>4));  /* show Principal variation */
    putled(2,'a'+(Y&7));putled(3,'8'-(Y>>4&7)); 
   }
#else   
   if(z&d>2)
   {printf("%2d ply, %9ld searched, score=%6d by %c%c%c%c\n",
     d-1,timer.w-(-128<<st),m,
     'a'+(X&7),'8'-(X>>4),'a'+(Y&7),'8'-(Y>>4&7)); /* uncomment for Kibitz */
   }
#endif     
 }                                             /*    encoded in X S,8 bits */
 k^=24;                                        /* change sides back        */
 ++MP;DD=m+=m<e;                               /* delayed-loss bonus       */
R:if (MP!=SA+U) switch(a){case 0:goto R0;case 1:goto R1;}
 OFFGRN();
}
