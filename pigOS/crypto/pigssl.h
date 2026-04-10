#pragma once
// pigOS pigssl - Crypto subsystem
#include "../drivers/vga/vga.h"
#include "../kernel/mem.h"

static void cmd_openssl(const char*a){
    if(!a||!*a){
        vset(C_LCYAN,C_BLACK); vpln("pigssl 1.0 - pigOS SSL/TLS/Crypto"); vrst();
        vpln("Commands: version enc dec genrsa x509 hash");
        return;
    }
    if(!ksc(a,"version")){vpln("pigssl 1.0.0 (pigOS built-in)");vpln("AES-256-GCM ChaCha20 RSA-2048 [simulated]");}
    else if(!ksnc(a,"genrsa",6)){vpln("Generating RSA-2048...");vpln("......++++");vpln("..........++++");vpln("[simulated - no hardware math]");}
    else if(!ksnc(a,"x509",4)){vpln("Certificate:");vpln("  Subject: CN=pigOS");vpln("  Issuer: CN=pigCA");vpln("  Valid: 1970-2099 [self-signed]");}
    else vpln("[pigssl: command queued - TCP stack needed for TLS]");
}
