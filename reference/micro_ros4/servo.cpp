// 包含舵机的头文件
#include <Servo.h>
// 新建 6 个舵机对象
Servo s1,s2,s3,s4,s5,s6;
// 最大最小脉冲范围
int minpulse=500;
int maxpulse=2500;
void setup(){
    // 初始化让这些舵机脉调整范围
    s1.attach(3,minpulse,maxpulse);
    s2.attach(5,minpulse,maxpulse);
    s3.attach(6,minpulse,maxpulse);
    s4.attach(9,minpulse,maxpulse);
    s5.attach(10, minpulse, maxpulse);
    s6.attach(11, minpulse, maxpulse);
    // 等待舵机初始化完成
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

