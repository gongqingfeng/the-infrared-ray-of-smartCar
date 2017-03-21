#include<reg51.h>
sbit PWM1 = P1^0;//左轮电机驱动使能
sbit PWM2 = P1^5;//右轮电机驱动使能
sbit motor_control_1 = P1^1;//左轮前进
sbit motor_control_2 = P1^2;//左轮后退
sbit motor_control_3 = P1^3;//右轮前进
sbit motor_control_4 = P1^4;//右轮后退

#define Imax 14000
#define Imin 8000
#define Inum1 1450
#define Inum2 700
#define Inum3 3000
unsigned char f=0;
unsigned char Im[4]={0x00,0x00,0x00,0x00} ;
unsigned char show[2]={0,0}	 ;
unsigned long Tc , m;
unsigned char IrOK;
unsigned char ucLock = 0;//互斥量，俗称原子锁



unsigned int uiPWMCnt1 = 0;	 //速度计数位
unsigned int uiPWM1 = 225;	  //默认速度值
unsigned int uiPWMCnt2 = 0;
unsigned int uiPWM2 = 225;

unsigned char ucTempPWM=225;//设置中间变量



void initial_myself();
void initial_peripheral();
void T0_time();
void usart_service(void);
void delay_long(unsigned int uiDelayLong);
void go_forward(void);//前进
void fall_back(void);//后退
void turn_left(void);//左转
void turn_right(void);//右转
void stop();//刹车
//延时函数
void delay(unsigned int k);

//外部中断解码程序
void intersvr0(void) interrupt 1  using 1
{
	Tc= TH0*256+TL0;
	TH0=0;
	TL0=0;
	if(Tc>Imin&&Tc<Imax)
	{
		m=0;
		f=1;
		return;	
	}
	  if(f==1)
	  {
	  	  if(Tc>Inum1&&Tc<Inum3)
		  {
		  	   Im[m/8] =Im[m/8]>>1|0x80;
			   m++;		  
		  }
	  	 if(Tc>Inum2&&Tc<Inum1)
		 {
		   Im[m/8] =Im[m/8]>>1;
		 	 m++;	
		 }
		if(m==32)
		{
			 m=0;
			 f=0;
			 if(Im[2]==~Im[3])
			 {
			 	 IrOK=1;
			 }	 else 	IrOK=0;		
		}
	  }
}




void main()
{			
        initial_myself();
        delay_long(100);
        initial_peripheral();
        while(1)
        {					
		  if(IrOK==1)
	      {
	   	   usart_service();	   
	   	   IrOK=0;	   
	      }  	                           
        }
}

//串口服务函数
void usart_service()
{					

                switch(Im[2])
                {	  
                      case 0x18:  //前进
                                Im[2] = 0x02;//避免一直触发
								
                                go_forward();
                                ucLock = 1;
                                uiPWM1 = uiPWM2 = ucTempPWM;
                                ucLock = 0;
                                break;
                    case  0x08  : //左转
                            Im[2] = 0x02;//避免一直触发
							
                                turn_left();
                                ucLock = 1;
                                uiPWM2 = ucTempPWM /4 *3;
                                uiPWM1 = ucTempPWM;
                                ucLock = 0;
                                break;
                    case 0x5A: //右转
                                Im[2] = 0x02;//避免一直触发
                                turn_right();
                                ucLock = 1;
                                uiPWM2 = ucTempPWM;
                                uiPWM1 = ucTempPWM /4 *3;
                                ucLock = 0;
                                break;
                     case 0x52 : //后退
                                Im[2] = 0x02;//避免一直触发
                                fall_back();
                                ucLock = 1;
                                uiPWM1 = uiPWM2 =ucTempPWM;
                                ucLock = 0;
                                break;
                     case 0x1C :   //停止
                                Im[2] = 0x02;//避免一直触发
                                stop();        
                                ucLock = 1;
                                uiPWM1 = uiPWM2 = ucTempPWM;
                                ucLock = 0;
                                break;				  		               
                        default : 
                                break;
        
                }
                delay_long(100);
                //速度调节的数据
                if(Im[2]!=0x18&&Im[2]!=0x08 && Im[2]!=0x5A &&  Im[2]!=0x52 && Im[2]!=0x1C&& Im[2]!=0x43&& Im[2]!=0x44&& Im[2]!=0x45&& Im[2]!=0x46&& Im[2]!=0x47&& Im[2]!=0x40)
                {
                        ucLock = 1;
							switch(Im[2])	 {							
								case 0x16: ucTempPWM = uiPWM1 = uiPWM2 =165;
								break;
								case 0x19: ucTempPWM = uiPWM1 = uiPWM2 =205;
								break;
								case 0x0D: ucTempPWM = uiPWM1 = uiPWM2 =255;
								break;
								 default : 
                                break;
							}

                        
                        ucLock = 0;
                }

}

void initial_myself()
{

      TMOD = 0x11;//设置定时器0为工作方式1
	    TH0=0;
	    TL0=0;
		  TH1 = 0xff;
      TL1 = 0x28; 
      TH0 = TL0 = 0xfd;     
      stop();
		  PWM1 = 1;
      PWM2 = 1;				 
 

}

void initial_peripheral()
{


        m=0;
        f=0;
        IT0=1;
        EX0=1;
        
      
        EA = 1;//开总中断
        ES = 1;//允许串口中断
        ET0 = 1;//允许定时器中断
        TR0 = 1;//启动定时器  
		    ET1 = 1;//允许定时器中断
        TR1 = 1;//启动定时器    
}

void T0_time() interrupt 3
{
        TF1 = 0;//清除中断标志
        TR1 = 0;//关定时器
        uiPWMCnt1 ++;
        uiPWMCnt2 ++;

        if(ucLock == 0)
        {
                if(uiPWMCnt1 > 255)
                {
                        uiPWMCnt1 = 0;
                }
                if(uiPWMCnt1 < uiPWM1)
                {
                        PWM1 = 1;
                        
                }
                else
                {
                        PWM1 = 0;
                }
        
                if(uiPWMCnt2 > 255)
                {
                        uiPWMCnt2 = 0;
                }
                if(uiPWMCnt2 < uiPWM2)
                {
                        PWM2 = 1;
                        
                }
                else
                {
                        PWM2 = 0;
                }
                                           
        
        }

        TH1 = 0xff;
        TL1 = 0x28;
        TR1 = 1;///开定时器

}

void delay(unsigned int k)
{
   unsigned int x,y;
     for(x=0;x<k;x++)
	 	for(y=0;y<2000;y++);
}
void delay_long(unsigned int uiDelayLong)
{
        unsigned int i;
        unsigned int j;
        for(i = 0 ; i < uiDelayLong ; i++)
        {
                for(j = 0; j < 500; j++);
        
        }

}
void stop()//停止
{
        
        motor_control_1 = 0;
        motor_control_2 = 0;
        motor_control_3 = 0;
        motor_control_4 = 0;
}
void fall_back()
{
       
        motor_control_1 = 0;
        motor_control_2 = 1;
        motor_control_3 = 0;
        motor_control_4 = 1;
}
void go_forward()
{
        
        motor_control_1 = 1;
        motor_control_2 = 0;
        motor_control_3 = 1;
        motor_control_4 = 0;
}
void turn_left()//左转
{
       
        motor_control_1 = 0;
        motor_control_2 = 0;
        motor_control_3 = 1;
        motor_control_4 = 0;
        
}
void turn_right()//右转
{
       
        motor_control_1 = 1;
        motor_control_2 = 0;
        motor_control_3 = 0;
        motor_control_4 = 0;
}