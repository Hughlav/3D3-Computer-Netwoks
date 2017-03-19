//
//  random ascii char.c
//
//
//  Created by Hugh Lavery on 11/02/2017.
//  14313812
//

#include <stdio.h>
#include <stdlib.h>

int RandomChar(int num){
    
    if (num == 0){ //returns ascii number in number range
        num = (rand()%10)+48;
    }
    
    if (num == 1){ //returns ascii number for uppercase letters
        num = (rand()%26)+65;
    }
    
    if (num == 2){ //returns ascii number for uppercase letters
        num = (rand()%26)+97;
    }
    return num;
}

int main(){
    FILE *fp;
    
    fp = fopen("ASCIIdata.txt", "w");
    int number = 0;
    for (int i=0; i<1024; i++){
        number = rand()%3;
        fputc(RandomChar(number), fp);
    }
    return 0;
}
