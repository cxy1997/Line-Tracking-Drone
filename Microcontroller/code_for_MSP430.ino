#define LEFT_AHEAD 9  //左轮向前转
#define LEFT_BACK 10  //左轮向后转
#define RIGHT_AHEAD 12  //右轮向前转
#define RIGHT_BACK 13  //左轮向后转
#define STEER 14  //转向舵方向
float MIDDLE = 0.863;  //转向舵中心位置
int RUN1 = 120;  //前进速度
int RUN2 = 100;  //转向速度
void left();  //左转
void right();  //右转
void advance();  //前进
void stop();  //停止
void retreat();  //后退
void setup()  //初始化
{
    Serial.begin(9600);//打开串口
    pinMode(LEFT_AHEAD,OUTPUT);
    pinMode(LEFT_BACK, OUTPUT);
    pinMode(RIGHT_AHEAD, OUTPUT);
    pinMode(RIGHT_BACK, OUTPUT);
    pinMode(STEER, OUTPUT);
    digitalWrite(LEFT_AHEAD, LOW);
    digitalWrite(LEFT_BACK, LOW);
    digitalWrite(RIGHT_AHEAD, LOW);
    digitalWrite(RIGHT_BACK, LOW);
    steer(MIDDLE);
}
char signal = ' ';  //每次读入一位字符
void loop()  //主循环
{
    if (Serial.available() > 0)
    {
        signal = Serial.read();
        if (signal == 'A') left(); 
        else if (signal == 'D') right();
        else if (signal == 'W') advance();
        else if (signal == 'S') retreat();
        else if (signal == 'Q') stop();
    }
}
void steer(float x)  //转向
{
    int tmp = x*1430;
    for(int i=0; i <5; i++)
    {
        digitalWrite(STEER,HIGH);
        delayMicroseconds(tmp);
        digitalWrite(STEER,LOW);
        delayMicroseconds(10000-tmp);
        delayMicroseconds(6800);
    }
}
void advance()  //前进
{
    steer(MIDDLE);
    digitalWrite(RIGHT_BACK,LOW);
    digitalWrite(LEFT_BACK,LOW);
    analogWrite(RIGHT_AHEAD,RUN1);
    analogWrite(LEFT_AHEAD,RUN1);
}
void left()  //左转
{
    steer(MIDDLE+0.15);
    digitalWrite(RIGHT_BACK,LOW);
    digitalWrite(LEFT_AHEAD,LOW);
    analogWrite(RIGHT_AHEAD,RUN2);
    analogWrite(LEFT_BACK,RUN2);
}
void right()  //右转
{
    steer(MIDDLE-0.15);
    digitalWrite(LEFT_BACK,LOW);
    digitalWrite(RIGHT_AHEAD,LOW);
    analogWrite(LEFT_AHEAD,RUN2);
    analogWrite(RIGHT_BACK,RUN2);
}
void stop()  //停止
{
    digitalWrite(LEFT_BACK, LOW);
    digitalWrite(RIGHT_BACK, LOW);
    digitalWrite(LEFT_AHEAD, LOW);
    digitalWrite(RIGHT_AHEAD, LOW);
    steer(MIDDLE);
}
void retreat()  //后退
{
    steer(MIDDLE);
    digitalWrite(RIGHT_AHEAD,LOW);
    digitalWrite(LEFT_AHEAD,LOW);
    analogWrite(RIGHT_BACK,RUN1);
    analogWrite(LEFT_BACK,RUN1);
}
