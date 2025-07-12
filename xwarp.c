/* copyright 1996, Kevin L. Suffecool, all rights reserved
**
** Toy XLIB program written for an X-Windows Programming course,
**    demonstrating the following key Xlib concepts:
**
**    o  real-time window updating
**    o  non-blocking event loop
**    o  various graphics primitives
**    o  setting and re-coloring cursors
**
** TO COMPILE:  cc xwarp.c -lX11 -s -o xwarp
**
*/
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

unsigned char torp1_bits[] = {
   0x10, 0x10, 0x10, 0x1f, 0xf8, 0x08, 0x08, 0x08
};

unsigned char torp2_bits[] = {
   0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81
};

void     srand48();     /* to seed our RNG */
double   drand48();     /* our RNG to use */

#define  NUMSTARS 100
struct   STAR  {
   int   x, y;          /* the star's (x,y) coord */
   int   vel;           /* star's velocity (1-5) */
   int   size;          /* either 1=SMALL or 2=LARGE */
   int   streak_len;    /* overall length of warp streak */
} stars[NUMSTARS];
#define  MAX_STREAK  10

#define  SMALL    1
#define  LARGE    2

#define  WIDTH    300
#define  HEIGHT   300

#define  MAXTORPS 5
struct   TORP  {
   int   x, y, flag;
} torp[MAXTORPS];

main()
{
   Display  *dsp;
   Window   win;
   Colormap cmap;
   Pixmap   starPix, torp_bmap[2];
   u_long   black, white, yellow, red;
   Cursor   trek;
   XColor   fg, bg;
   GC    pixGC, winGC;
   Bool  warp = False;
   int   i, y, scr, streak_count = 0;

   /* get some default stuff
   */
   dsp = XOpenDisplay( NULL );
   scr = DefaultScreen( dsp );
   black = BlackPixel( dsp, scr );
   white = WhitePixel( dsp, scr );
   cmap = DefaultColormap( dsp, scr );

   /* create our Window and the Window's GC
   */
   win = XCreateSimpleWindow( dsp, RootWindow(dsp,scr), 0, 0,
                              WIDTH, HEIGHT, 2, white, black );
   winGC = XCreateGC( dsp, win, 0, NULL );
   XSetGraphicsExposures( dsp, winGC, False );
   XSetBackground( dsp, winGC, black );

   /* create the Enterprise cursor and recolor it
   */
   trek = XCreateFontCursor( dsp, XC_trek );
   XDefineCursor( dsp, win, trek );
   XParseColor( dsp, cmap, "white", &fg );
   XParseColor( dsp, cmap, "black", &bg );
   XRecolorCursor( dsp, trek, &fg, &bg );

   /* create the two photon-torpedo bitmaps
   */
   torp_bmap[0] = XCreateBitmapFromData( dsp, win, (const char *)torp1_bits, 8, 8 );
   torp_bmap[1] = XCreateBitmapFromData( dsp, win, (const char *)torp2_bits, 8, 8 );

   XAllocNamedColor( dsp, cmap, "yellow", &fg, &fg );
   yellow = fg.pixel;
   XAllocNamedColor( dsp, cmap, "red", &fg, &fg );
   red = fg.pixel;

   /* create and clear our starfield Bitmap
   */
   starPix = XCreatePixmap( dsp, win, WIDTH, HEIGHT, 1 );
   pixGC = XCreateGC( dsp, starPix, 0, NULL );
   XSetForeground( dsp, pixGC, 0 );
   XFillRectangle( dsp, starPix, pixGC, 0, 0, WIDTH, HEIGHT );

   /* initialize our starfield with some stars
   ** (one tenth are of the LARGE variety)
   */
   srand48( (long)getpid() );
   for ( i = 0; i < NUMSTARS; i++ )
   {
      stars[i].x = (int)(drand48() * WIDTH);
      stars[i].y = (int)(drand48() * HEIGHT);
      stars[i].vel = 1 + (int)(5.0 * drand48());
      stars[i].size = (drand48() < 0.9)?  SMALL : LARGE;
      stars[i].streak_len = (2 * stars[i].vel * MAX_STREAK);
   }

   /* kludgey method to give our window a titlebar name
   */
   {
      XTextProperty  xtp;
      char  *winname = "Space, the Final Frontier ...";
      XStringListToTextProperty( &winname, 1, &xtp );
      XSetWMName( dsp, win, &xtp );
   }

   /* make sure we set our minimum and maximum window sizes;
   ** (don't allow the user to resize the window to be larger)
   */
   {
      XSizeHints  *hints;
      hints = XAllocSizeHints();
      hints->flags = PMinSize|PMaxSize;
      hints->min_width = hints->min_height = 100;
      hints->max_width = WIDTH;
      hints->max_height = HEIGHT;
      XSetWMNormalHints( dsp, win, hints );
      XFree( hints );
   }

   XSelectInput( dsp, win, ButtonPressMask | ButtonReleaseMask | ExposureMask );
   XMapWindow( dsp, win );

   while ( 1 )
   {
      while ( XPending( dsp ) )
      {
         XEvent   ev;
         XNextEvent( dsp, &ev );    /* guaranteed NOT to block! */

         if ( ev.type == ButtonPress  ||  ev.type == ButtonRelease )
         {
            if ( ev.xbutton.button == Button3 )
            {
               if ( ev.type == ButtonPress )
                  warp = True, streak_count = 0;
               else
                  warp = False;
            }
            else if ( ev.type == ButtonPress  &&  ev.xbutton.button == Button1 )
            {
               for ( i = 0; i < MAXTORPS  &&  torp[i].y > 0; i++ );
               if ( i < MAXTORPS )
               {
                  torp[i].x = ev.xbutton.x - 4;
                  torp[i].y = ev.xbutton.y;
               }
            }
            else if ( ev.type == ButtonRelease  &&  ev.xbutton.button == Button2
 )
               exit( 0 );
         }
      }

      if ( warp )
      {
         if ( streak_count < MAX_STREAK )
         {
            /* we're entering warp drive, so we need to streak
            ** our stars for MAX_STREAK iterations
            */
            streak_count++;
            XSetForeground( dsp, pixGC, 0 );
            XFillRectangle( dsp, starPix, pixGC, 0, 0, WIDTH, HEIGHT );
            XSetForeground( dsp, pixGC, 1 );
            for ( i = 0; i < NUMSTARS; i++ )
            {
               stars[i].y += streak_count;
               y = stars[i].y + (streak_count * 2 * stars[i].vel);
               if ( stars[i].size == SMALL )
                  XDrawLine( dsp, starPix, pixGC, stars[i].x, stars[i].y,
                           stars[i].x, y );
               else
                  XFillRectangle( dsp, starPix, pixGC, stars[i].x, stars[i].y,
                           3, y - stars[i].y );
               if ( stars[i].y > HEIGHT )
                  stars[i].y -= (HEIGHT + stars[i].streak_len);
            }
         }
         else
         {
            /* we're in warp drive, but we've already streaked
            ** our stars; therefore, we just need to move the
            ** streaks down the screen (a little faster)
            */
            XSetForeground( dsp, pixGC, 0 );
            XFillRectangle( dsp, starPix, pixGC, 0, 0, WIDTH, HEIGHT );
            XSetForeground( dsp, pixGC, 1 );
            for ( i = 0; i < NUMSTARS; i++ )
            {
               stars[i].y += (3 * stars[i].vel);
               y = stars[i].y + stars[i].streak_len;
               if ( stars[i].size == SMALL )
                  XDrawLine( dsp, starPix, pixGC, stars[i].x, stars[i].y,
                           stars[i].x, y );
               else
                  XFillRectangle( dsp, starPix, pixGC, stars[i].x, stars[i].y,
                           3, y - stars[i].y );
               if ( stars[i].y > HEIGHT )
               {
                  stars[i].y = -stars[i].streak_len;
                  stars[i].x = (int)(drand48() * WIDTH);
               }
            }
         }
      }
      else if ( streak_count )
      {
         /* we're coming out of warp drive; reverse the streaking
         ** process for MAX_STREAK iterations
         */
         if ( streak_count == MAX_STREAK )
            for ( i = 0; i < NUMSTARS; i++ )
               stars[i].y += (2 * stars[i].vel * MAX_STREAK);

         streak_count--;
         XSetForeground( dsp, pixGC, 0 );
         XFillRectangle( dsp, starPix, pixGC, 0, 0, WIDTH, HEIGHT );
         XSetForeground( dsp, pixGC, 1 );
         for ( i = 0; i < NUMSTARS; i++ )
         {
            y = stars[i].y - (2 * stars[i].vel * streak_count);
            if ( stars[i].size == SMALL )
               XDrawLine( dsp, starPix, pixGC, stars[i].x, y,
                        stars[i].x, stars[i].y );
            else
               XFillRectangle( dsp, starPix, pixGC, stars[i].x, y,
                        3, stars[i].y - y );
         }
      }
      else
      {
         /* space-normal speed; move stars normally
         */
         XSetForeground( dsp, pixGC, 0 );
         XFillRectangle( dsp, starPix, pixGC, 0, 0, WIDTH, HEIGHT );
         XSetForeground( dsp, pixGC, 1 );
         for ( i = 0; i < NUMSTARS; i++ )
         {
            stars[i].y += stars[i].vel;
            if ( stars[i].y > HEIGHT )
            {
               stars[i].x = (int)(drand48() * WIDTH);
               stars[i].y -= HEIGHT;
            }
            if ( stars[i].size == SMALL )
               XDrawPoint( dsp, starPix, pixGC, stars[i].x, stars[i].y );
            else
               XFillArc( dsp, starPix, pixGC, stars[i].x, stars[i].y,
                           3, 3, 0, 360*64 );
         }
      }

      /* redraw the starfield
      */
      if ( warp  ||  streak_count )
         XSetForeground( dsp, winGC, red );
      else
         XSetForeground( dsp, winGC, white );
      XCopyPlane( dsp, starPix, win, winGC, 0, 0, WIDTH, HEIGHT, 0, 0, 1 );

      /* handle any photon torpedos
      */
      for ( i = 0; i < MAXTORPS; i++ )
         if ( torp[i].y > 0 )
         {
            torp[i].y -= 5;
            torp[i].flag = 1 - torp[i].flag;
            XSetForeground( dsp, winGC, yellow );
            XCopyPlane( dsp, torp_bmap[torp[i].flag], win,
                  winGC, 0, 0, 8, 8, torp[i].x, torp[i].y, 1 );
         }

      usleep( 20000 );  /* nighty-nite */
   }
}
