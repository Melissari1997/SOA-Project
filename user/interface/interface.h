#ifndef interface_h
#define interface__h

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <TAG_MACROS.h>

extern int sysCallIndex[4];
int convertStrtoArr(char *str, int numArr[], int limit);

void initTBDE();

// @Return CREATE:
//  succes            :=    tag descriptor
//  ETOOMANYREFS      :=    numero di TAG Service oltre il limite
//  EBADR             :=    Chiave già in uso
//  EFAULT            :=    Errore generico
// --
// @Return OPEN:
//  succes            :=    tag descriptor
//  EBADSLT           :=    Chiave privata
//  EBADRQC           :=    Permission invalid
//  ENOMSG            :=    Chiave non trovata
int tag_get(int key, int command, int permission);

// @Return tag_send:
//  succes            :=    return 0
//  EXFULL            :=    Buffer troppo grande(>MAX_BUF_SIZE) o non specificato
//  ENODATA           :=    Problema nella trasmissione al modulo kernel
//  ENOMSG            :=    Tag non trovato
//  EBADRQC           :=    Permission invalid
//  EBADSLT           :=    Livello > levelDeep
//  ENOSYS            :=    sysCall rimossa
//  EILSEQ            :=    comando non valido
//  EBADMSG           :=    Errore nella list-replace
int tag_send(int tag, int level, char *buffer, size_t size);

// @Return tag_receive:
//  succes            :=    return size(message)
//  EXFULL            :=    Buffer troppo grande (> MAX_BUF_SIZE), o non presente
//  ENOMSG            :=    Tag non trovato
//  EBADRQC           :=    Permessi non validi per eseguire l'operazione
//  EBADSLT           :=    Livello > levelDeep
//  ERESTART          :=    thread svegliato da un segnale
//  EUCLEAN           :=    thread svegliato dal comando AWAKE_ALL
//  ENOSYS            :=    sysCall rimossa
//  EILSEQ            :=    comando non valido
int tag_receive(int tag, int level, char *buffer, size_t size);

// @Return AWAKE_ALL:
//  succes            :=    return 0
//  ENOMSG            :=    Nessun thread da svegliare
//  EBADRQC           :=    Permessi non validi per eseguire l'operazione
// @Return REMOVE:
//  succes            :=    return 0
//  ENODATA           :=    Il tag specificato non esiste
//  EBADE             :=    Permessi non validi per eseguire l'operazione
//  EADDRINUSE        :=    Reader in attesa in qualche livello
int tag_ctl(int tag, int command);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void tagGet_perror(int keyAsk, int commandAsk);
void tagSend_perror(int tag);
void tagRecive_perror(int tag);
void tagCtl_perror(int tag, int commandAsk);
#endif
