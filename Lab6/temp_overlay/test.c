#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#define SIZE 20

struct DataItem {
   char data[30];   
   char key[30];
};

struct DataItem* hashArray[SIZE]; 
struct DataItem* dummyItem;
struct DataItem* item;

int hashCode(char* key) {
	
	unsigned long int hashval =0;
	int i = 0;
	
	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		//hashval = hashval << 8;
		hashval += key[ i ];
		i++;
		//printf("hashval: %ld,i: %d\n",hashval,i);
	}
	
	printf("key: %s, hash_key: %ld\n",key,hashval % SIZE);
	return hashval % SIZE;
	
   //return key % SIZE;
}

struct DataItem *search(char* key) {
   //get the hash 
   int hashIndex = hashCode(key);  
   //printf("inside search!: %d\n",hashIndex);
   //printf("hashArray[hashIndex]->key: %s, key: %s\n",hashArray[hashIndex]->key,key);
   //move in array until an empty 
   while(hashArray[hashIndex] != NULL) {
	
      if(strcmp(hashArray[hashIndex]->key,key)==0)
         return hashArray[hashIndex]; 
			
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }        
	
   return NULL;        
}

void insert(char* key,char* data) {

   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   strcpy(item->data,data);     
   strcpy(item->key,key);
   item->data[strlen(item->data)]='\0';
   item->key[strlen(item->key)]='\0';

   //get the hash 
   int hashIndex = hashCode(key);

   //move in array until an empty or deleted cell
   while(hashArray[hashIndex] != NULL ) {
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }
	
   hashArray[hashIndex] = item;
}

struct DataItem* delete(char* key) {
   
   //get the hash 
   int hashIndex = hashCode(key);

   //move in array until an empty
   while(hashArray[hashIndex] != NULL) {
	
      if(strcmp(hashArray[hashIndex]->key,key)==0) {
         struct DataItem* temp = hashArray[hashIndex]; 
			
         //assign a dummy item at deleted position
         hashArray[hashIndex] = NULL; 
         return temp;
      }
		
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }      
	
   return NULL;        
} 

void display() {
   int i = 0;
	
   for(i = 0; i<SIZE; i++) {
	
      if(hashArray[i] != NULL)
         printf(" (%s,%s)\n",hashArray[i]->key,hashArray[i]->data);
      else
         printf(" ~~\n");
   }
	
   printf("\n");
}

int main() {
   /* dummyItem = (struct DataItem*) malloc(sizeof(struct DataItem));
   dummyItem->data = -1;  
   dummyItem->key = -1;  */

   insert("128.10.25.101:20025", "128.10.25.102:20026");
   insert("128.10.25.102:20026", "128.10.25.101:20025");
   insert("128.10.25.103:20027", "128.10.25.104:20028");
   insert("128.10.25.104:20028", "128.10.25.103:20027");
   insert("128.10.25.105:20029", "128.10.25.102:20026");
/*    insert(42, 80);
   insert(4, 25);
   insert(12, 44);
   insert(14, 32);
   insert(17, 11);
   insert(13, 78);
   insert(37, 97);
 */
   display();
   item = search("128.10.25.101:20025");

   if(item != NULL) {
      printf("Element found: %s\n", item->data);
   } else {
      printf("Element not found\n");
   }

   delete("128.10.25.101:20025");
   display();
   item = search("128.10.25.101:20025");

   if(item != NULL) {
      printf("Element found: %s\n", item->data);
   } else {
      printf("Element not found\n");
   } 
}