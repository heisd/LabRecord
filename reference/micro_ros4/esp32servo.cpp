#include <ESP32Servo.h>
// 创建舵机对象
Servo s1,s2,s3,s4,s5,s6;
//设置最大和最小的脉冲范围
int minpulse = 500;
int maxpulse = 2500;
// 这里的引脚要看集线器和我们的 esp32 的连接情况，按需修改，第一位是引脚
void setup() { 
    s1.attach(18,minpulse,maxpulse);
    s2.attach(19,minpulse,maxpulse);
    s3.attach(21,minpulse,maxpulse);
    s4.attach(22,minpulse,maxpulse);
    s5.attach(23,minpulse,maxpulse);
    s6.attach(25,minpulse,maxpulse);
    delay(1000);
}
// 控制 6 个舵机的参数,在 arduino 的 servo 库里面这些参数代表这个轴转多少度
void moveArm(int a,int b,int c,int d, int e ,int f){
    s1.write(a);
    s2.write(b);
    s3.write(c);        
    s4.write(d);
    s5.write(e);
    s6.write(f);
}
// 这里旋转角度是一个显示的例子，按需更改，单位是角度，可以参考上一个写的示例
void loop(){
    // 让六个轴依次摆动
    moveArm(30,40,60,20,50,70);
    delay(1500);
    moveArm(90, 80, 120, 60, 90, 30);
    delay(1500);
    moveArm(150, 40, 100, 120, 60, 150);
    delay(1500);
    moveArm(90, 90, 90, 90, 90, 90);
    delay(1500);
}