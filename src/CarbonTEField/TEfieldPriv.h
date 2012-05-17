
#define TEXTMARGIN			4
#define MAX_CHARS_IN_FIELD	32767
#define S_BAR_WIDTH 16

/* standard things */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NIL
#define NIL (void *)0
#endif

#ifndef CH_ENTER
#define CH_ENTER 0x03						/* Character code for enter key */
#endif
#ifndef CH_BS
#define CH_BS 0x08							/* Character code for backspace (delete) */
#endif
#ifndef CH_CR
#define CH_CR '\r'							/* Character code for carriage return  */
#endif
#ifndef CLEARKEY
#define CLEARKEY 27							/* Character code for clear key */
#endif
#ifndef CMD_KEY
#define CMD_KEY 17							/* Character code for command key */
#endif

#ifndef LEFTARROWKEY
#define LEFTARROWKEY	28						/* Character codes for arrow keys */
#endif
#ifndef RIGHTARROWKEY
#define RIGHTARROWKEY 29
#endif
#ifndef UPARROWKEY
#define UPARROWKEY 30
#endif
#ifndef DOWNARROWKEY
#define DOWNARROWKEY 31
#endif

#ifndef PGUPKEY								/* Character codes for some ext. kybd. keys */
#define PGUPKEY 0x0B
#endif
#ifndef PGDNKEY
#define PGDNKEY 0x0C
#endif
#ifndef HOMEKEY
#define HOMEKEY 0x01
#endif
#ifndef ENDKEY
#define ENDKEY 0x04
#endif
#ifndef FORWARD_DEL_CHAR
#define FORWARD_DEL_CHAR 127
#endif

#ifndef CTL_ACTIVE
#define CTL_ACTIVE 0
#endif
#ifndef CTL_INACTIVE
#define CTL_INACTIVE 255
#endif

