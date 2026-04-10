// pigOS pi/pivi editor - separate from pnano
// Arrow keys for movement, i=insert, ESC=command, :wq=save+quit, :q!=quit

#define VI_ROWS 20
#define VI_COLS 78
#define VI_BUF_MAX 4096

typedef enum { VI_NORMAL, VI_INSERT, VI_CMD } ViMode;

static void cmd_vi(const char*f){
    char path[256]="/tmp/unnamed.txt";
    if(f&&*f) fs_resolve(sh_cwd,f,path,256);

    // Load content
    char vi_buf[VI_BUF_MAX]="";
    int vi_len=0;
    MemFile*mf=memfs_find(path);
    if(mf){kcp(vi_buf,mf->content);vi_len=ksl(vi_buf);}
    else{const VNode*n=fs_find_vnode(path);if(n&&n->content&&ksl(n->content)<VI_BUF_MAX-1){kcp(vi_buf,n->content);vi_len=ksl(vi_buf);}}

    // Split into lines
    char lines[VI_ROWS+4][VI_COLS];
    int nlines=1; int col=0;
    for(int i=0;i<VI_ROWS+3;i++) lines[i][0]=0;
    for(int i=0;i<vi_len&&nlines<VI_ROWS+2;i++){
        if(vi_buf[i]=='\n'){ lines[nlines-1][col]=0; nlines++; col=0; }
        else if(col<VI_COLS-1){ lines[nlines-1][col++]=(char)vi_buf[i]; lines[nlines-1][col]=0; }
    }

    int cy=0,cx=0; // cursor row,col
    ViMode mode=VI_NORMAL;
    char cmd_buf[64]=""; int cmd_len=0;
    int modified=0;

    // Draw function
    #define VI_DRAW() do{ \
        vclear(); \
        uint8_t tb_vi=COL(C_BLACK,C_LGREY); \
        vfill(0,0,VGA_W,1,' ',tb_vi); \
        vstr(1,0,tb_vi,"  pi - "); vstr(7,0,tb_vi,path); \
        if(modified){vstr(VGA_W-12,0,COL(C_RED,C_LGREY)," [Modified]");} \
        uint8_t mc_vi=mode==VI_INSERT?COL(C_WHITE,C_BLUE):COL(C_WHITE,C_BLACK); \
        vx=0; vy=1; /* Start below title bar */ \
        for(int r=0;r<VI_ROWS&&r<nlines;r++){ \
            vset(C_DGREY,C_BLACK); char lb[4]; kia(r+1,lb); \
            int ll=ksl(lb); while(ll++<3)vpc(' '); vps(lb); vpc(' '); \
            vset(C_WHITE,C_BLACK); vps(lines[r]); vpc('\n'); \
        } \
        /* Status bar */ \
        vfill(0,VGA_H-2,VGA_W,1,' ',COL(C_BLACK,C_CYAN)); \
        if(mode==VI_NORMAL) vstr(1,VGA_H-2,COL(C_BLACK,C_CYAN),"NORMAL  i=insert  :w=save  :q=quit  :wq=save+quit  dd=del  /=search"); \
        else if(mode==VI_INSERT) vstr(1,VGA_H-2,COL(C_WHITE,C_BLUE),"-- INSERT -- ESC=normal  arrows=move"); \
        else if(mode==VI_CMD){ vstr(1,VGA_H-2,COL(C_BLACK,C_CYAN),":"); vstr(2,VGA_H-2,COL(C_BLACK,C_CYAN),cmd_buf); } \
        /* Cursor */ \
        int draw_cx=4+cx; int draw_cy=1+cy; \
        if(draw_cx<VGA_W&&draw_cy<VGA_H-2){ \
            uint16_t cell=VGA_MEM[draw_cy*VGA_W+draw_cx]; \
            uint8_t col2=(uint8_t)(cell>>8); \
            uint8_t fg2=col2&0xF,bg2=(col2>>4)&0xF; \
            VGA_MEM[draw_cy*VGA_W+draw_cx]=(cell&0xFF)|((uint16_t)((fg2<<4)|bg2)<<8); \
        } \
        vga_update_hw_cursor(); \
    }while(0)

    VI_DRAW();

    while(1){
        int c=kb_get();
        if(mode==VI_NORMAL){
            if(c=='i'){mode=VI_INSERT;}
            else if(c==':'){ mode=VI_CMD; cmd_len=0; cmd_buf[0]=0; }
            else if(c=='h'||c==KEY_LEFT){ if(cx>0)cx--; }
            else if(c=='l'||c==KEY_RIGHT){ if(cx<(int)ksl(lines[cy])-1)cx++; }
            else if(c=='k'||c==KEY_UP){ if(cy>0){cy--;int l=ksl(lines[cy]);if(cx>l)cx=l;} }
            else if(c=='j'||c==KEY_DOWN){ if(cy<nlines-1){cy++;int l=ksl(lines[cy]);if(cx>l)cx=l;} }
            else if(c=='0'){ cx=0; }
            else if(c=='$'){ cx=ksl(lines[cy]); }
            else if(c=='G'){ cy=nlines-1; }
            else if(c=='g'){ cy=0; cx=0; }
            else if(c=='d'){ // dd - delete line
                int c2=kb_get();
                if(c2=='d'&&nlines>1){
                    for(int r=cy;r<nlines-1;r++) kcp(lines[r],lines[r+1]);
                    lines[nlines-1][0]=0; nlines--; modified=1;
                    if(cy>=nlines) cy=nlines-1;
                }
            }
            else if(c=='o'){ // open line below
                if(nlines<VI_ROWS+2){
                    for(int r=nlines;r>cy+1;r--) kcp(lines[r],lines[r-1]);
                    lines[cy+1][0]=0; nlines++; cy++; cx=0; mode=VI_INSERT; modified=1;
                }
            }
            else if(c=='O'){ // open line above
                if(nlines<VI_ROWS+2){
                    for(int r=nlines;r>cy;r--) kcp(lines[r],lines[r-1]);
                    lines[cy][0]=0; nlines++; cx=0; mode=VI_INSERT; modified=1;
                }
            }
        } else if(mode==VI_INSERT){
            if(c==27){ mode=VI_NORMAL; if(cx>0)cx--; }
            else if(c==KEY_LEFT){ if(cx>0)cx--; }
            else if(c==KEY_RIGHT){ if(cx<(int)ksl(lines[cy]))cx++; }
            else if(c==KEY_UP){ if(cy>0)cy--; }
            else if(c==KEY_DOWN){ if(cy<nlines-1)cy++; }
            else if(c==KEY_HOME){ cx=0; }
            else if(c==KEY_END){ cx=ksl(lines[cy]); }
            else if(c=='\b'){
                if(cx>0){
                    int l=ksl(lines[cy]);
                    for(int i=cx-1;i<l;i++) lines[cy][i]=lines[cy][i+1];
                    cx--; modified=1;
                } else if(cy>0){
                    // Merge with previous line
                    int plen=ksl(lines[cy-1]);
                    kcat(lines[cy-1],lines[cy]);
                    for(int r=cy;r<nlines-1;r++) kcp(lines[r],lines[r+1]);
                    lines[nlines-1][0]=0; nlines--;
                    cy--; cx=plen; modified=1;
                }
            }
            else if(c=='\n'){
                if(nlines<VI_ROWS+2){
                    for(int r=nlines;r>cy+1;r--) kcp(lines[r],lines[r-1]);
                    // Split current line at cx
                    kcp(lines[cy+1],lines[cy]+cx);
                    lines[cy][cx]=0; nlines++; cy++; cx=0; modified=1;
                }
            }
            else if(c>=32&&c<127){
                int l=ksl(lines[cy]);
                if(l<VI_COLS-2){
                    for(int i=l;i>cx;i--) lines[cy][i]=lines[cy][i-1];
                    lines[cy][cx]=(char)c; lines[cy][l+1]=0;
                    cx++; modified=1;
                }
            }
        } else if(mode==VI_CMD){
            if(c==27){ mode=VI_NORMAL; cmd_len=0; cmd_buf[0]=0; }
            else if(c=='\n'){
                // Execute command
                if(!ksc(cmd_buf,"q!")||!ksc(cmd_buf,"q")) goto vi_done;
                if(!ksc(cmd_buf,"w")||!ksc(cmd_buf,"wq")||!ksc(cmd_buf,"x")){
                    // Save
                    char out_buf[VI_BUF_MAX]="";
                    for(int r=0;r<nlines;r++){
                        kcat(out_buf,lines[r]);
                        if(r<nlines-1) kcat(out_buf,"\n");
                    }
                    MemFile*m2=memfs_find(path);
                    if(!m2)m2=memfs_create(path);
                    if(m2)kcp(m2->content,out_buf);
                    modified=0;
                    if(!ksc(cmd_buf,"wq")||!ksc(cmd_buf,"x")) goto vi_done;
                }
                mode=VI_NORMAL; cmd_len=0; cmd_buf[0]=0;
            }
            else if(c=='\b'){ if(cmd_len>0){cmd_len--;cmd_buf[cmd_len]=0;} }
            else if(c>=32&&c<127&&cmd_len<63){ cmd_buf[cmd_len++]=(char)c; cmd_buf[cmd_len]=0; }
        }
        VI_DRAW();
    }
vi_done:
    vclear(); vrst();
}
