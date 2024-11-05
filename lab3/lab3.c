#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "asm-arm/arch-pxa/lib/creator_pxa270_lcd.h"

int stack_num[100] = {0}, stack_op[100] = {0};
int op_ptr = 0, num_ptr = 0;
int state = 0;

ushort parser(ushort key, lcd_write_info_t *display){
    //  +-*/
    if(key >= 'A'){
        if(state){
            if( op_ptr > 0 && stack_op[op_ptr-1] > 1){
                num_ptr--;
                op_ptr--;
                stack_num[num_ptr-1] = (stack_op[op_ptr] == 2) ? stack_num[num_ptr-1] * stack_num[num_ptr] : stack_num[num_ptr-1] / stack_num[num_ptr];
                // printf("stack_num refresh: %d\n", stack_num[num_ptr-1]);
            }
        }
        state = 0;
        stack_op[op_ptr] = key - 'A';
        op_ptr++;
        switch(key){
            case 'A': return '+';
            case 'B': return '-';
            case 'C': return '*';
            case 'D': return '/';
        }
    }
    // 0~9
    else if (key >= '0'){
        if(state){
            stack_num[num_ptr-1] = stack_num[num_ptr-1] * 10 + key - '0';
        }
        else {
            stack_num[num_ptr] = key - '0';
            num_ptr++;
        }
        state = 1;
        return key;
    }
    else{
        display->Count=0;
        
        // AC
        if(key == '*'){
            op_ptr = 0;
            num_ptr = 0;
            state = 0;
            return 0;
        }
        // CAL
        else{
            if( op_ptr > 0 && stack_op[op_ptr-1] > 1){
                num_ptr--;
                op_ptr--;
                stack_num[num_ptr-1] = (stack_op[op_ptr] == 2) ? stack_num[num_ptr-1] * stack_num[num_ptr] : stack_num[num_ptr-1] / stack_num[num_ptr];
            }

            int i;
            for(i=0;i<op_ptr;i++)
                stack_num[i+1] = stack_num[i] + (float)(0.5-stack_op[i]) * 2 * stack_num[i+1];

            // printf("Ans: %d\n", stack_num[num_ptr-1]);

            while(stack_num[num_ptr-1]>0) {
                (display->Msg)[display->Count] = (stack_num[num_ptr - 1] % 10) + '0';
                stack_num[num_ptr - 1] /= 10;
                display->Count++;
            }
            
            for(i=0;i<(display->Count)/2;i++){
                (display->Msg)[i] ^= (display->Msg)[display->Count - i - 1];
                (display->Msg)[display->Count - i - 1] ^= (display->Msg)[i];
                (display->Msg)[i] ^= (display->Msg)[display->Count - i - 1];
            }

            op_ptr = 0;
            num_ptr = 0;
            state = 0;
            return '=';
        }
    }
}

void update_lcd(int fd, ushort c, lcd_write_info_t *display){
    if (c == 0){
        ushort i = LED_ALL_OFF;
        ioctl(fd, LED_IOCTL_SET, &i);
        ioctl(fd, _7SEG_IOCTL_ON, NULL);
        ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    }
    else if(c == '='){
        ushort LED=0;
        int i;
        _7seg_info_t data;  
        data.Mode = _7SEG_MODE_HEX_VALUE;
        data.Which = _7SEG_ALL;
        data.Value = 0;
        for(i=0;i<display->Count ;i++){
            data.Value = (data.Value << 4) + (display->Msg)[i] - '0';
            LED = LED * 10 + ((display->Msg)[i] - '0');
        }
        ioctl(fd, _7SEG_IOCTL_SET, &data);
        for(i=0;i<8;i++){
            if((LED >> i) & 1) ioctl(fd, LED_IOCTL_BIT_SET, &i);
        }
        for(i=(display->Count)-1;i>=0;i--)(display->Msg)[i+1] = (display->Msg)[i];
        display->Count++;
        (display->Msg)[0] = c;
        ioctl(fd , LCD_IOCTL_WRITE, display);


    }
    else{
        (display->Msg)[0] = c;
        display->Count = 1;
        ioctl(fd , LCD_IOCTL_WRITE, display);
    }
}

int main(){
    
    int i, j, fd;
    if((fd = open("/dev/lcd", O_RDWR)) < 0){
        printf("Open faild\n");
        exit(-1);
    }
    i = LED_ALL_OFF;
    ioctl(fd, LED_IOCTL_SET, &i);
    ioctl(fd, _7SEG_IOCTL_ON, NULL);
    ioctl(fd, LCD_IOCTL_CLEAR, NULL);
    printf("clean done!\n");

    lcd_write_info_t display;
    display.Count = 0;
    display.CursorX = 10;
    display.CursorY = 5;
    unsigned short key;
    while(1){
        
        ioctl(fd, KEY_IOCTL_WAIT_CHAR, &key);
        // printf("I: %c\n", key & 0xff);
        ushort c = parser(key & 0xff, &display);
        update_lcd(fd, c, &display);
        ioctl(fd, KEY_IOCTL_CLEAR, key);
    }
}
