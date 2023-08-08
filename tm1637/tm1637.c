#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wiringx.h>
#include <unistd.h>
#include <time.h>
#define I2C_DEV "/dev/i2c-1"
#define I2C_ADDR 0x68

#define I2C_SCL 4
#define I2C_SDA 5

#define CLK 2
#define DIO 3

char segdata[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};  // LOW-9
char segdatap[] = {0xbf, 0x86, 0xdb, 0xcf, 0xe6, 0xed, 0xfd, 0x87, 0xff, 0xef}; // 带小数可以实现时间

void Init_I2C(void)
{
}

void tm1637_start()
{
    digitalWrite(CLK, HIGH);
    usleep(140);
    digitalWrite(DIO, HIGH);
    usleep(140);
    digitalWrite(DIO, LOW);
    usleep(140);
    digitalWrite(CLK, LOW);
    usleep(140);
}

void tm1637_stop()
{
    digitalWrite(CLK, LOW);
    usleep(140);
    digitalWrite(DIO, LOW);
    usleep(140);
    digitalWrite(CLK, HIGH);
    usleep(140);
    digitalWrite(DIO, HIGH);
    usleep(140);
}

void write_bit(char bit)
{
    digitalWrite(CLK, LOW);
    usleep(140);
    if (bit)
    {
        digitalWrite(DIO, HIGH);
    }
    else
    {
        digitalWrite(DIO, LOW);
    }
    usleep(140);
    digitalWrite(CLK, HIGH);
    usleep(140);
}

void write_byte(char data) // 写字节
{
    for (int i = LOW; i < 8; i++)
    {
        write_bit((data >> i) & 0x01);
    }
    digitalWrite(CLK, LOW);
    usleep(140);
    digitalWrite(DIO, HIGH);
    usleep(140);
    digitalWrite(CLK, HIGH);
    usleep(140);

    pinMode(DIO, PINMODE_INPUT);
    while (digitalRead(DIO))
        ;

    pinMode(DIO, PINMODE_OUTPUT);
}

void write_command(char cmd) // 写命令
{
    tm1637_start();
    write_byte(cmd);
    tm1637_stop();
}

void write_data(char addr, char data) // 写值
{
    tm1637_start();
    write_byte(addr);
    write_byte(data);
    tm1637_stop();
}

void time_dislaly(int h_shi, int h_ge, int m_shi, int m_ge, int ctrl) // 显示
{
    write_command(0x40); // 写数据
    write_command(0x44); // 固定地址
    write_data(0xc0, segdata[h_shi]);
    if (!ctrl)
    {
        write_data(0xc1, segdata[h_ge]);
    }
    else
    {
        write_data(0xc1, segdatap[h_ge]);
    }
    write_data(0xc2, segdata[m_shi]);
    write_data(0xc3, segdata[m_ge]);
    write_command(0x8F); // 显示开
}

void tm1637_init()
{
}

uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

void read_ds3231(int fd, struct tm *data, uint8_t len)
{
    // printf("read start");
    int dat[8] = {0};
    wiringXI2CWrite(fd, 0x00);
    for (int i = 0; i < len; i++)
    {
        dat[i] = wiringXI2CRead(fd);
    }
    data->tm_hour = bcd2bin(dat[2]);
    data->tm_min = bcd2bin(dat[1]);
    data->tm_sec = bcd2bin(dat[0]);
    // printf("read end");
}

void write_ds3231(int fd, struct tm *td, uint8_t len)
{
    wiringXI2CWriteReg8(fd, 0x00, 0);
    printf("start\r\n");
    wiringXI2CWriteReg8(fd, 0, bin2bcd(td->tm_sec));
    printf("sec %x\r\n", bin2bcd(td->tm_sec));
    wiringXI2CWriteReg8(fd, 1, bin2bcd(td->tm_min));
    printf("min %x\r\n", bin2bcd(td->tm_min));
    wiringXI2CWriteReg8(fd, 2, bin2bcd(td->tm_hour));
    printf("hour %x\r\n", bin2bcd(td->tm_hour));
}

void getTemperature(int fd, uint16_t *temp)
{
    wiringXI2CWrite(fd, 0x11);
    int dat[3] = {0};
    dat[0] = wiringXI2CRead(fd);
    dat[1] = wiringXI2CRead(fd);
    *temp = (dat[0] + (dat[1] >> 6) * 0.25) * 100;
}

int main()
{
    time_t timer;
    struct tm *t;
    // uint8_t read_data[8] = {0};
    int fd_i2c;
    // tm1637_init(); // 初始化

    if (wiringXSetup("duo", NULL) == -1)
    {
        wiringXGC();
        return -1;
    }

    if ((fd_i2c = wiringXI2CSetup(I2C_DEV, I2C_ADDR)) < 0)
    {
        printf("I2C Setup failed: %i\n", fd_i2c);
        return -1;
    }

    pinMode(CLK, PINMODE_OUTPUT);
    pinMode(DIO, PINMODE_OUTPUT);
    uint16_t tmp = 0;
    uint8_t cnt = 0;
    while (1)
    {

        if (10 > cnt++)
        {
            read_ds3231(fd_i2c, t, 8);
            // printf("%d:%d:%d\r\n", t->tm_hour, t->tm_min, t->tm_sec);
            time_dislaly(t->tm_hour / 10, t->tm_hour % 10, t->tm_min / 10, t->tm_min % 10, t->tm_sec % 2);
        }
        else
        {
            if (cnt >= 12)
            {
                cnt = 0;
            }
            getTemperature(fd_i2c, &tmp);
            time_dislaly(tmp / 1000, (tmp / 100 % 10) - 2, (tmp % 100) / 10, (tmp % 100) % 10, 1);
            // printf("Temperature:%d.%d\r\n", tmp / 100, tmp % 100);
            tmp = 0;
        }
        sleep(1);
    }
}
