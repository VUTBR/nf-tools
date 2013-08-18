///Program: lpm.c
///Autor: Martin Ministr
///Datum: 10.2.2013
//

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#define LPM_MAX_INSTANCES 1024


#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define BUFFSIZE 60
#define ALOCSIZE 100000

const int lastAlocIndex = ALOCSIZE - 1;  

/// Structure of Node in Trie
typedef struct TrieNode{
  struct TrieNode *pTN0;
  struct TrieNode *pTN1;
  SV * Value;
  bool hasValue;
} TTrieNode;

/// Structure of alokated chunks of Trie Nodes
typedef struct AlocTrieNodes{
  struct TrieNode TrieNodes[ALOCSIZE];
  struct AlocTrieNodes *pNextATN;
} TAlocTrieNodes;


/* instance data */
typedef struct lpm_instance_s
{
	TTrieNode *pTrieIPV4;
	TTrieNode *pTrieIPV6;
} lpm_instance_t;


/* array of initalized instances */
lpm_instance_t *lpm_instances[LPM_MAX_INSTANCES] = { NULL };


/// function Prototypes

void addPrefixToTrie(unsigned char *prefix, unsigned char prefixLen, SV * Value,  TTrieNode **ppTrie);
TTrieNode *createTrieNode();
void freeAlocTrieNodes(TAlocTrieNodes *pATN);
TTrieNode *lookupAddress(unsigned char *address, int addrLen, TTrieNode *pTN);
TTrieNode **lookupInTrie(unsigned char *prefix, unsigned char *byte, unsigned char *bit, unsigned char *makeMatches, TTrieNode **ppTN, bool *root); 
TTrieNode *myMalloc();


/// Alocation speeding variables
TAlocTrieNodes *pAlocated = NULL;
TAlocTrieNodes *pActual = NULL;
struct TrieNode *pActualTN = NULL;
struct TrieNode *pLastTN = NULL;

/// IPV4 Trie
TTrieNode *pTrieIPV4 = NULL;

/// IPV6 Trie
TTrieNode *pTrieIPV6 = NULL;

/* clean tree and decrement reference count */
void freeTrieNode(TTrieNode *pTN) {
	if (pTN != NULL) {
		if (pTN->hasValue) {
			pTN->hasValue = false;
			SvREFCNT_dec(pTN->Value);
		}
		freeTrieNode(pTN->pTN0);
		freeTrieNode(pTN->pTN1);
		free(pTN);
	}
}

/**
 * addPrefixToTrie adds prefix to Trie
 * prefix - prefix that will be added to Trie
 * prefixLen - prefix length
 * ASNum - number of AS
 * ppTrie - pointer at pointer of desired Trie(IPV4 or IPV6)
 * returns error if fails
 */
void addPrefixToTrie(unsigned char *prefix, unsigned char prefixLen, SV * Value, TTrieNode **ppTrie){
  unsigned char byte = 0;
  unsigned char bit = 128; 
  
  unsigned char makeMatches = prefixLen;
  bool root = false;
  TTrieNode *pFound = (*ppTrie);
  TTrieNode **ppTN = lookupInTrie(prefix, &byte, &bit, &makeMatches, &pFound, &root);
  if(root){
    ppTN = ppTrie;
  }
  if(ppTN != NULL){
    while(makeMatches){
      (*ppTN) = createTrieNode();
      unsigned char unmasked = (prefix[byte] & bit);
      
      if(unmasked){
        ppTN = &((*ppTN)->pTN1);        
      }
      else{
        ppTN = &((*ppTN)->pTN0);
      }
      
      makeMatches--;
      bit = (bit >> 1);
      if(!bit){
        byte++;
        bit = 128;
      }
    }
    
    (*ppTN) = createTrieNode();
    (*ppTN)->Value = Value;
    (*ppTN)->hasValue = true;
	SvREFCNT_inc(Value);
  }
  else{
    if(!pFound->hasValue){      
      pFound->Value = Value;
      pFound->hasValue = true;
	  SvREFCNT_inc(Value);
    }
  }
}

/**
 * createTrieNode creates Node of Trie and sets him implicitly up
 * returns NULL or created Node of Trie
 */
TTrieNode *createTrieNode(){
  //TTrieNode *pTN = myMalloc(sizeof(struct TrieNode));
  TTrieNode *pTN = malloc(sizeof(struct TrieNode));
  if(pTN == NULL){
    fprintf(stderr, "pTN malloc error.");
    return NULL;
  }
  
  // initialize
  pTN->pTN0 = NULL;
  pTN->pTN1 = NULL;
  pTN->hasValue = false;
  
  return pTN;
}


/**
 *  freeAlocTrieNodes
 *  pATN - pointer at structure to be freed  
 */ 
void freeAlocTrieNodes(TAlocTrieNodes *pATN){
  if(pATN->pNextATN != NULL){
    freeAlocTrieNodes(pATN->pNextATN);  
  }  
  free(pATN);
}

/** 
 * lookupAddressIPv6
 * address - field with address
 * addrLen - length of IP adress in bits (32 for IPv4, 128 for IPv6)
 * returns NULL when NOT match adress to any prefix in Trie
 */
TTrieNode *lookupAddress(unsigned char *address, int addrLen, TTrieNode *pTN){ 
  unsigned char byte = 0;
  unsigned char bit = 128; // most significant bit 
//  TTrieNode *pTN = pTrieIPV6;
  TTrieNode *pTNValue = NULL; 
  
  unsigned char addrPassed = 0;
  while(addrPassed++ <= addrLen){
    unsigned char unmasked = (address[byte] & bit);
    bit = (bit >> 1);    
    if(!bit){
      byte++;
      bit = 128;
    }
    
    // pTN with ASNum is desired
    if(pTN->hasValue){
      pTNValue = pTN;
    }
    
    if(unmasked){
      if(pTN->pTN1 != NULL){
        pTN = pTN->pTN1;
      }
      else{
        return pTNValue;
      }                   
    }
    else{
      if(pTN->pTN0 != NULL){
        pTN = pTN->pTN0;
      }
      else{
        return pTNValue;
      }       
    }           
  }       
  return pTNValue;
}

/** 
 * lookupInTrie
 * prefix - holds the prefix that is searched during Trie building
 * byte - byte of prefix
 * bit - bit of prefix byte
 * makeMatches - input and output,indicates prefixLen that can be used
 * ppTN - input and output, determines which Node of Trie was examined last during function proccess, at start holds the root of Trie  
 * root - return flag, true if root of Trie must be build first
 * returns NULL when match current prefix during Trie building
 */
TTrieNode **lookupInTrie(unsigned char *prefix, unsigned char *byte, unsigned char *bit, unsigned char *makeMatches, TTrieNode **ppTN, bool *root){
  if((*ppTN) == NULL){
    (*root) = true;
    return ppTN;
  }
  
  while((*makeMatches)){    
    unsigned char unmasked = (prefix[(*byte)] & (*bit));
    (*makeMatches)--;
    (*bit) = ((*bit) >> 1);
    if(!(*bit)){
      (*byte)++;
      (*bit) = 128;
    }
    
    if(unmasked){
      if((*ppTN)->pTN1 != NULL){
        (*ppTN) = (*ppTN)->pTN1;
      }
      else{
        return &((*ppTN)->pTN1);
      }                      
    }
    else{
      if((*ppTN)->pTN0 != NULL){
        (*ppTN) = (*ppTN)->pTN0;
      }
      else{
        return &((*ppTN)->pTN0);
      }    
    }           
  }
  
  (*root) = false;
  return NULL;
}

/**
 * myMalloc
 * encapsulates real malloc function but call it less times 
 * and that is why it could save some presious time
 */ 
TTrieNode *myMalloc(){
  if(pAlocated == NULL){
    pAlocated = malloc(sizeof(struct AlocTrieNodes));
    if(pAlocated == NULL){
      return NULL;
    }
    
    pAlocated->pNextATN = NULL;
    pActual = pAlocated;
    pActualTN = &pActual->TrieNodes[0];
    pLastTN = &pActual->TrieNodes[lastAlocIndex]; 
  }
  
  // Save to return it later
  TTrieNode *pReturn = pActualTN;
  
  if(pActualTN == pLastTN){
    pActual->pNextATN = malloc(sizeof(struct AlocTrieNodes));
    pActual = pActual->pNextATN;
    if(pActual == NULL){
      return NULL;
    }
    
    pActual->pNextATN = NULL;
    pActualTN = &pActual->TrieNodes[0];
    pLastTN = &pActual->TrieNodes[lastAlocIndex];
  }
  else{
    pActualTN++;
  }
  
  return pReturn;  
}


/********************************************************************/
/* PERL INTERFACE                                                   */
/********************************************************************/

int lpm_init(void) {
int handle = 1;
lpm_instance_t *instance;
//int i;

	/* find the first free handler and assign to array of open handlers/instances */
	while (lpm_instances[handle] != NULL) {
		handle++;
		if (handle >= LPM_MAX_INSTANCES - 1) {
			croak("No free handles available, max instances %d reached", LPM_MAX_INSTANCES);
			return 0;
		}
	}

	instance = malloc(sizeof(lpm_instance_t));
	if (instance == NULL) {
		croak("can not allocate memory for instance");
		return 0;
	}

	memset(instance, 0, sizeof(lpm_instance_t));

	instance->pTrieIPV4 = NULL;
	instance->pTrieIPV6 = NULL;

    lpm_instances[handle] = instance;
	
	return handle;
}

int lpm_add(int handle, char *prefix, SV *value) {
lpm_instance_t *instance = lpm_instances[handle];
//char *strPrefix = NULL;
char *strPrefixLen = NULL;
unsigned int prefixLen;
unsigned char buf[16];
int i = 0;

	if (instance == NULL ) {
		croak("handler %d not initialized");
		return 0;
	}

//	strPrefix = prefix;
	while (prefix[i] != '/' && prefix[i] != '\0') {
		i++;	
		if (prefix[i] == '\0') {
			strPrefixLen = prefix + i;
		} else if (prefix[i] == '/') {
			prefix[i] = '\0';
			strPrefixLen = prefix + i + 1;
		}
	}
        
	if(prefix == NULL || strPrefixLen == NULL){
		croak("Invalid prefix %s %s", prefix, strPrefixLen);
		return 0;
	}

	prefixLen = atoi(strPrefixLen);

	if(inet_pton(AF_INET, prefix, buf)){
		if (strPrefixLen[0] == '\0') { 
			prefixLen = 32;
		}
		addPrefixToTrie(buf, prefixLen, value, &instance->pTrieIPV4);
	}
	else if(inet_pton(AF_INET6, prefix, buf)){ // IPV6
		if (strPrefixLen[0] == '\0') { 
			prefixLen = 128;
		}
		addPrefixToTrie(buf, prefixLen, value, &instance->pTrieIPV6);
	}
	else{ // Corrupted input file
		croak("Cannot add prefix %s", prefix);
		return 0;
	}
	
	/* included code */
	return 1;
}

SV *lpm_lookup(int handle, char *straddr) {
lpm_instance_t *instance = lpm_instances[handle];
//SV *out = NULL;
//char buf[BUFFSIZE];
//char *buf = NULL;
unsigned char addr[16];

	if (instance == NULL ) {
		croak("handler %d not initialized");
		return NULL;
	}

	TTrieNode *pTN = NULL;
    
    if(inet_pton(AF_INET, straddr, addr)){
      pTN = lookupAddress(addr, 32, instance->pTrieIPV4);
    }
    else if(inet_pton(AF_INET6, straddr, addr)){ // IPV6
      pTN = lookupAddress(addr, 128, instance->pTrieIPV6);
    }

    if ( pTN == NULL ){
		return &PL_sv_undef;
    } else {
		SvREFCNT_inc(pTN->Value);
		return pTN->Value;
    }
}

SV *lpm_lookup_raw(int handle, SV *svaddr) {
lpm_instance_t *instance = lpm_instances[handle];
//SV *out = NULL;
//char buf[BUFFSIZE];
//char *buf = NULL;
char *addr;
STRLEN len;

	if (instance == NULL ) {
		croak("handler %d not initialized");
		return NULL;
	}

	TTrieNode *pTN = NULL;
	addr = SvPV(svaddr, len);
	 
    if(len == 4){
      pTN = lookupAddress((void *)addr, 32, instance->pTrieIPV4);
    }
    else if(len == 16){ // IPV6
      pTN = lookupAddress((void *)addr, 128, instance->pTrieIPV6);
    } 

    if ( pTN == NULL ){
		return &PL_sv_undef;
    } else {
		SvREFCNT_inc(pTN->Value);
		return pTN->Value;
    }
}

void lpm_finish(int handle) {
lpm_instance_t *instance = lpm_instances[handle];

	if (instance == NULL ) {
		croak("handler %d not initialized");
		return;
	}

	if (instance->pTrieIPV4 != NULL) {
		freeTrieNode(instance->pTrieIPV4);
		instance->pTrieIPV4 = NULL;
	}

	if (instance->pTrieIPV6 != NULL) {
		freeTrieNode(instance->pTrieIPV6);
		instance->pTrieIPV6 = NULL;
	}
	return;
}


void lpm_destroy(int handle) {
lpm_instance_t *instance = lpm_instances[handle];

	if (instance == NULL ) {
		croak("handler %d not initialized");
		return;
	}

//	freeAlocTrieNodes(instance->pTrieIPV4);
//	freeAlocTrieNodes(instance->pTrieIPV6);

	lpm_finish(handle);
	free(instance);
	lpm_instances[handle] = NULL;

	return;
}


