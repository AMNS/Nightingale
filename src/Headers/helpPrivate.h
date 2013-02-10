/***********************************************************************\
| Purpose :: This header file contains all of the global data 			|
|			 structures and function prototypes necessary for this		|
|			 program to run correctly.	It is also designed to compile  |
|			 cleanly with help.c, in order to make a code library file. |
|-----------------------------------------------------------------------|
|		Copyright © Joe Pillera, 1990.  All Rights Reserved.			|
|		Revised by Don Byrd, 1997.										|
\***********************************************************************/


/* Maximum number of topics */
#define		MAX_TOPICS			64
#define		TOPIC_NAME_LENGTH	63		/* Max. chars. in a topic name */

/* Dialog box information */
#define		Help_Window			5300
#define		OK_Button			1
#define		Topics_Area			2
#define		Display_Area		3
#define		Next_Button			4
#define		Prev_Button			5
#define		Message_Area		7

/* Constant for first screen in a series */
#define		Initial_Picture			0

/* Are we in picture or text mode? */
typedef  enum {
	pict,
	text
}  ViewMode;

/* topic name limited to TOPIC_NAME_LENGTH char pascal string */
typedef unsigned char TopicStr[TOPIC_NAME_LENGTH+1];

/* Errors I check for - MacTutor Help is very forgiving! */
typedef  enum {
	err_no_HTBL,		/* Missing ID=128 of the help table	  */
	err_min_res,		/* Not enough help screens specified  */
	err_intro_pict,		/* Intro screen must be a picture	  */
	err_bad_type,		/* Not a TEXT or PICT screen (typo?)  */
	err_no_pict,		/* The PICT for this topic isnt there */
	err_no_text			/* The TEXT for this topic isnt there */
}  ErrorTypes;

/* Read the HTBL resource information */
typedef struct {
	int			First_Menu;	
	int			Last_Menu;	
	int			Number_Of_Topics;	
	long		type;
	int			resource_start;
	int			resource_end;
	TopicStr	topic_name;
} **Help_Info_Handle;

/* Other constants */
#define		NOT_FOUND			-1
#define		ON					0
#define		OFF					255
#define		FRONT				(WindowPtr)-1
#define		RMARGIN				3

/* Function prototypes */
extern	void	Add_List_String( TopicStr *, ListHandle );
extern	void	Bold_Button( DialogPtr, short );
extern	void	Center_Window( DialogPtr, short, Boolean );
extern	Boolean Create_Help( short, long );
extern	void	Display_Pict( short );
extern  void	Display_Text( void );
extern	void	Do_Help( short, long );
extern	void	Error_Message( ErrorTypes );
extern	void	Help_Event_Loop( void );
extern	void	Handle_List_Event( short );
extern	void	Handle_Update( void );
extern	void	Init_Help( void );
extern	void	Kill_Window( void );
extern  pascal  void My_Scroll_Filter ( ControlHandle, short );
extern  short 	ParseInt( char ** );
extern	long	ParseOSType( char **);
extern	void 	ParseString( char *, char ** );
extern  void	pStrCat(const Str255, const Str255, Str255);
extern	void	pStrCopy( char *, char * );
extern 	void  	Refresh_Topics( void );
extern  void	Scroll_Picture( short );
extern  void	Scroll_Text( short, short );
extern	void	Set_Button_State( short, Boolean );
extern  void	User_Message( const unsigned char * );
extern	void	DrawBorder( void );
