#pragma once
// pigOS pigfetch - modern system info
// Colors matched to output.txt
#include "../drivers/vga/vga.h"
#include "../kernel/mem.h"
#include "../drivers/net/network.h"

static void cmd_pigfetch_dmesg(){
    vset(C_LGREEN,C_BLACK);
    vpln("[  0.000] POUG: multiboot loaded pigKernel");
    vpln("[  0.001] pigKernel 1.0 x86_64 Long Mode");
    vpln("[  0.002] VGA: 80x25 CP437 @ 0xB8000");
    vpln("[  0.003] PS/2: keyboard + mouse initialized");
    vpln("[  0.004] PCI: RTL8139 found at 00:03.0");
    extern int net_ok;
    if(!net_ok){ vset(C_YELLOW,C_BLACK); vpln("[  0.005] WARN: RTL8139 no carrier"); }
    vset(C_LGREEN,C_BLACK);
    vpln("[  0.006] poing: 13 units started");
    vpln("[  0.007] larpshell: tty0 ready");
}

static void cmd_pigfetch(){
    vclear();
    vx=0; vy=0;
    int art_x=0, info_x=32;

    // Row 0: lddo:xOnWWx - ALL RED (no yellow anywhere on head)
    vstr(0,0,COL(C_LRED,C_BLACK),"ld");
    vstr(2,0,COL(C_LRED,C_BLACK),"do:");
    vstr(14,0,COL(C_LRED,C_BLACK),"xOnWWx");
    
    // Row 1: :kxxxdl;xOkkOkkOo - ALL RED (no yellow, no xO)
    vstr(2,1,COL(C_DGREY,C_BLACK),":");
    vstr(3,1,COL(C_LRED,C_BLACK),"kxxx");
    vstr(7,1,COL(C_LRED,C_BLACK),"dl");
    vstr(19,1,COL(C_LRED,C_BLACK),";xOkkOkkOo");
    vstr(info_x,1,COL(C_LCYAN,C_BLACK),"OS:      pigOS v1.0 x86_64");

    // Row 2: cxxxxdxdxO00OxddOOOOkxkko - YELLOW on only 3 O's on right ear
    vstr(2,2,COL(C_LRED,C_BLACK),"cxxxxdxdxO00Oxdd");
    vstr(16,2,COL(C_YELLOW,C_BLACK),"OOO");
    vstr(19,2,COL(C_LRED,C_BLACK),"kko");
    vstr(info_x,2,COL(C_LCYAN,C_BLACK),"Kernel:  pigKernel 1.0 (x86_64)");

    // Row 3: 'xxxxxkkOOOOOkkxxkOOkOkx0' - YELLOW only on last two O's at position 18-19
    vstr(1,3,COL(C_DGREY,C_BLACK),"'");
    vstr(2,3,COL(C_LRED,C_BLACK),"xxxxxkkOOOOOkkxxkOOkOkx0");
    vstr(18,3,COL(C_YELLOW,C_BLACK),"OO");
    vstr(20,3,COL(C_LRED,C_BLACK),"0");
    vstr(info_x,3,COL(C_LCYAN,C_BLACK),"Init:    poing v1.0");

    // Row 4: lxxxxdxkkOOkdddxxxOOOkOx k, - YELLOW only on three O's
    vstr(1,4,COL(C_LRED,C_BLACK),"lxxxxdxkkOO");
    vstr(11,4,COL(C_LRED,C_BLACK),"kdddxxx");
    vstr(18,4,COL(C_YELLOW,C_BLACK),"OOO");
    vstr(21,4,COL(C_LRED,C_BLACK),"kOx");
    vstr(24,4,COL(C_LRED,C_BLACK)," k");
    vstr(25,4,COL(C_DGREY,C_BLACK),",");
    vstr(info_x,4,COL(C_LCYAN,C_BLACK),"Shell:   larpshell v5.9.3");

    // Row 5: 'xko;dkkkkkd;oxxddxOOOkK - YELLOW only on three O's
    vstr(1,5,COL(C_DGREY,C_BLACK),"'");
    vstr(2,5,COL(C_LRED,C_BLACK),"xk");
    vstr(4,5,COL(C_BROWN,C_BLACK),"o");
    vstr(5,5,COL(C_DGREY,C_BLACK),";");
    vstr(6,5,COL(C_LRED,C_BLACK),"dkkkkkd");
    vstr(12,5,COL(C_DGREY,C_BLACK),";");
    vstr(13,5,COL(C_BROWN,C_BLACK),"oxxddx");
    vstr(19,5,COL(C_YELLOW,C_BLACK),"OOOk");
    vstr(23,5,COL(C_LRED,C_BLACK),"K");
    vstr(info_x,5,COL(C_LCYAN,C_BLACK),"WM:      PigWM");

    // Row 6: dxxxkkkkkkkkkxxdddxx'
    vstr(1,6,COL(C_LRED,C_BLACK),"dxxxkkkkkkkkkxxddd");
    vstr(18,6,COL(C_LRED,C_BLACK),"x");
    vstr(19,6,COL(C_DGREY,C_BLACK),"x");
    vstr(20,6,COL(C_DGREY,C_BLACK),"'");
    vstr(info_x,6,COL(C_LCYAN,C_BLACK),"Boot:    POUG");

    // Row 7: 'xxxxxxlxxlddxxxxxxxx,
    vstr(1,7,COL(C_DGREY,C_BLACK),"'");
    vstr(2,7,COL(C_LRED,C_BLACK),"xxxxxxlxxlddxxxxxxxx");
    vstr(21,7,COL(C_DGREY,C_BLACK),",");
    vstr(info_x,7,COL(C_LCYAN,C_BLACK),"CPU:     x86_64 Long Mode");

    // Row 8: xxxxdl;dd::oodxxxxx:
    vstr(1,8,COL(C_LRED,C_BLACK),"xxxx");
    vstr(5,8,COL(C_LRED,C_BLACK),"dl");
    vstr(7,8,COL(C_DGREY,C_BLACK),";");
    vstr(8,8,COL(C_LRED,C_BLACK),"dd");
    vstr(10,8,COL(C_DGREY,C_BLACK),"::");
    vstr(12,8,COL(C_LRED,C_BLACK),"oodxxxxx");
    vstr(19,8,COL(C_DGREY,C_BLACK),":");
    vstr(info_x,8,COL(C_LCYAN,C_BLACK),"Memory:  4MB heap @ 0x400000");

    // Row 9: xxxdl   lodxxxd
    vstr(3,9,COL(C_LRED,C_BLACK),"xxx");
    vstr(6,9,COL(C_LRED,C_BLACK),"dl   l");
    vstr(11,9,COL(C_LRED,C_BLACK),"odxxxd");
    vstr(info_x,9,COL(C_LCYAN,C_BLACK),"Display: VGA 80x25 CP437");

    vset(C_LGREEN,C_BLACK);
    vy=11;
}