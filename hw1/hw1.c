#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "asm-arm/arch-pxa/lib/creator_pxa270_lcd.h"

char item[6][25] ={
    "cookie 60$",
    "cake 80$",
    "tea:40$",
    "boba:70$",
    "fried rice:120$",
    "Egg-drop soup 50$"
};


struct shopping_car{
    ushort shop_dis;
    ushort order_list[10];
    ushort number_list[10];
};

struct shopping_car order;
int _fd;
lcd_write_info_t *_display;

void print_LCD(lcd_write_info_t *display, const char *str){
    strncpy((char *)(display->Msg), str, strlen(str));
    display->Count = strlen(str);
    ioctl(_fd , LCD_IOCTL_WRITE, display);
}

ushort read_button(){
    ushort key;
    ioctl(_fd, KEY_IOCTL_WAIT_CHAR, &key);
    return key & 0xff;
}

ushort read_number(int mode){
    ushort num = 0;
    ushort key = 0;
    while(1){
        if(mode) print_LCD(_display, "How many?\n");//printf("How many?\n");
        ioctl(_fd, KEY_IOCTL_WAIT_CHAR, &key);
        key = key & 0xff;
        if (key == '#')break;
        num = num * 10 + (key - '0');
        // printf("%c\n", key);

        (_display->Msg)[0] = key;
        (_display->Msg)[1] = '\n';
        (_display->Count)= 2;
        ioctl(_fd , LCD_IOCTL_WRITE, _display);
    }
    // printf("ret: %u\n", num);
    return num;
}

void show_shop_list(lcd_write_info_t *display){

    ioctl(_fd, LCD_IOCTL_CLEAR, NULL);

    // printf("dessert shop: 3km\nBeverage Shop: 5km\nDiner: 8km\n");
    print_LCD(display, "dessert shop: 3km\nBeverage Shop: 5km\nDiner: 8km\n");
    read_button();
}

void show_menu(lcd_write_info_t *display){

    ioctl(_fd, LCD_IOCTL_CLEAR, NULL);

    // printf("Please choose 1~3\n1. Dessert shop\n2. Beverage Shop\n3. Diner\n");
    print_LCD(display, "Please choose 1~3\n1. Dessert shop\n2. Beverage Shop\n3. Diner\n");
    int n = read_number(0);
    switch(n){
        case 3:
            order.shop_dis += 3;
        case 2:
            order.shop_dis += 2;
        case 1:
            order.shop_dis += 3;
    }
    show_order(display, n);
}

void show_order(lcd_write_info_t *display, int n){
    
    _7seg_info_t data;
    data.Mode = _7SEG_MODE_HEX_VALUE;
    data.Which = _7SEG_ALL;
    int total = 0, i;

    while(1){
        ioctl(_fd, LCD_IOCTL_CLEAR, NULL);
        // printf("Please choose 1~4\n1.cookie 60$\n2.cake 80$\n3.confirm\n4.cancel\n");
        char buf[255]={0};
        // 0 -> 0/1 1-> 2/3 2 -> 4/5
        sprintf(buf, "Please choose 1~4\n1.%s\n2.%s\n3.confirm\n4.cancel\n", item[2*(n-1)], item[2*(n-1)+1]);
        print_LCD(display, buf);
        
        switch(read_number(0)){
            case 1:
                set_order_num(2*(n-1));
                break;
            case 2:
                set_order_num(2*(n-1)+1);
                break;
            case 3:
                // 開始送餐,led顯示進度;7段顯示器顯示金額:1140
                
                data.Value = 0;

                for(i=0;i<6;i++) total += order.order_list[i] * order.number_list[i];
                printf("total: %d\n", total);
                i=0;
                while(total>0){
                    data.Value += (total%10) << (4*i);
                    i++;
                    total /= 10;
                }
                ioctl(_fd, _7SEG_IOCTL_SET, &data);
                // printf("data.Value: %x\n", data.Value);
                // printf("Please wait for few minutes...\n");
                print_LCD(display, "Please wait for few minutes...\n");
                
                for(i=0;i<order.shop_dis;i++) ioctl(_fd, LED_IOCTL_BIT_SET, &i);
                while(order.shop_dis > 0){
                    ioctl(_fd, LED_IOCTL_BIT_CLEAR, &(order.shop_dis));
                    order.shop_dis--;
                    sleep(1);
                }
                i = LED_ALL_OFF;
                ioctl(_fd, LED_IOCTL_SET, &i);
                // 當餐點送達(led全滅)後
                // printf("please pick up your meal\n");
                print_LCD(display, "please pick up your meal\n");
                read_button();
                return;
            case 4:
                // system("cls");
                ioctl(_fd, LCD_IOCTL_CLEAR, NULL);
                return;
        }
    }
}

void set_order_num(int index){
    order.number_list[index] = read_number(1);
}

int main(){
    
    int i;
    if((_fd = open("/dev/lcd", O_RDWR)) < 0){
        printf("Open faild\n");
        exit(-1);
    }
    i = LED_ALL_OFF;
    ioctl(_fd, LED_IOCTL_SET, &i);
    ioctl(_fd, _7SEG_IOCTL_ON, NULL);
    ioctl(_fd, LCD_IOCTL_CLEAR, NULL);
    printf("clean done!\n");

    lcd_write_info_t display;
    display.Count = 0;
    display.CursorX = 10;
    display.CursorY = 5;
    _display = &display;

    order.order_list[0] = 60;
    order.order_list[1] = 80;
    order.order_list[2] = 40;
    order.order_list[3] = 70;
    order.order_list[4] = 120;
    order.order_list[5] = 50;

    while(1){
        ioctl(_fd, LCD_IOCTL_CLEAR, NULL);
        order.shop_dis = 0;
        for(i=0;i<6;i++)order.number_list[i] = 0;
        // printf("1. shop list\n2. order\n");
        print_LCD(&display, "1. shop list\n2. order\n");
        // print_LCD(&display, "2. order");
        switch(read_number(0)){
            
            case 1:
                show_shop_list(&display);
                break;
            case 2:
                show_menu(&display);
                break;  
        }
    }
}
