#include "Ano_FlightCtrl.h"
#include "Ano_Imu.h"
#include "Drv_icm20602.h"
#include "Ano_MagProcess.h"
#include "Drv_spl06.h"
#include "Ano_MotionCal.h"
#include "Ano_AttCtrl.h"
#include "Ano_LocCtrl.h"
#include "Ano_AltCtrl.h"
#include "Ano_MotorCtrl.h"
#include "Drv_led.h"
#include "Ano_RC.h"
#include "Drv_laser.h"
#include "Ano_OF.h"
#include "Ano_OF_DecoFusion.h"
#include "Ano_FlyCtrl.h"
#include "Ano_UWB.h"
#include "Ano_Sensor_Basic.h"
#include "Ano_Imu_Calibration.h"
//#include "Ano_DT.h"
//#include "Ano_LED.h"
#include "Ano_ProgramCtrl_User.h"
#include "Drv_OpenMV.h"
//#include "Ano_FcData.h"
# include "ultr.h"
#include "Drv_usart.h"
#include "Drv_OpenMV.h"




/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
u8  	search_flag;//����Ѱ�˱�־λ
u16   search_pole_flag =0;////Ѱ�˱�־λ
u16   turnstile_flag = 0;////�Ƹ˱�־λ
u16   pole_flag=0;///����˵ı�־λ

extern u16 usart_flag;
extern u16 start_flag;
extern u16 change_flag1;

/////////////////////////////////////////////////////////

/*PID������ʼ��*/
void All_PID_Init(void)
{

	/*��̬���ƣ����ٶ�PID��ʼ��*/
	Att_1level_PID_Init();
	
	/*��̬���ƣ��Ƕ�PID��ʼ��*/
	Att_2level_PID_Init();
	
	/*�߶ȿ��ƣ��߶��ٶ�PID��ʼ��*/
	Alt_1level_PID_Init();	
	
	/*�߶ȿ��ƣ��߶�PID��ʼ��*/
	Alt_2level_PID_Init();
	
	
	/*λ���ٶȿ���PID��ʼ��*/
	Loc_1level_PID_Init();
	
}

/*���Ʋ����ı�����*/
void ctrl_parameter_change_task()
{

	
	if(0)
	{
		Set_Att_2level_Ki(0);
		
	}
	else
	{
		if(flag.auto_take_off_land ==AUTO_TAKE_OFF)
		{

			Set_Att_1level_Ki(2);
		}
		else
		{

			Set_Att_1level_Ki(1);
		}
		
		Set_Att_2level_Ki(1);
	}
}


/*һ�����������ޣ�*/
void one_key_roll()
{

			if(flag.flying && flag.auto_take_off_land == AUTO_TAKE_OFF_FINISH)
			{	
				if(rolling_flag.roll_mode==0)
				{
					rolling_flag.roll_mode = 1;
					
				}
			}

}

//static u16 one_key_taof_start;
 u16 one_key_taof_start;


/*һ�����������Ҫ����Ϊ�ӳ٣�*/
void one_key_take_off_task(u16 dt_ms)
{
	if(one_key_taof_start != 0)
	{
		one_key_taof_start += dt_ms;
		
		
		if(one_key_taof_start > 1400 && flag.motor_preparation == 1)
		{
			one_key_taof_start = 0;
				if(flag.auto_take_off_land == AUTO_TAKE_OFF_NULL)
				{
					flag.auto_take_off_land = AUTO_TAKE_OFF;
					//���������

					flag.taking_off = 1;
				}
			
		}
	}
	//reset
	if(flag.unlock_sta == 0)
	{
		one_key_taof_start = 0;
	}

}
/*һ�����*/
/////////////////////////
void one_key_take_off()
{
	if(flag.unlock_err == 0)
	{	
		if(flag.auto_take_off_land == AUTO_TAKE_OFF_NULL && one_key_taof_start == 0)
		{
			one_key_taof_start = 1;
			flag.unlock_cmd = 1;
		}
	}
}


////////////////////////////////////////////////////////
//void one_key_take_off()
//{
//	if(flag.unlock_err == 0)
//	{	
//		
////		if(flag.auto_take_off_land == 1 && flag.motor_preparation == 1)

//		if(flag.auto_take_off_land == AUTO_TAKE_OFF_NULL && one_key_taof_start == 0)
//		{
////								flag.taking_off = 1;
////							loc_ctrl_2.exp[2]= 150;
////			flag.auto_take_off_land = AUTO_TAKE_OFF_NULL
//			
//			one_key_taof_start = 1;
//			flag.unlock_cmd = 1;
//		}
//	}
//}


/////////////////////////////////////////////////////////////
/*һ������*/
void one_key_land()
{
	flag.auto_take_off_land = AUTO_LAND;
}

//////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////
_flight_state_st fs;

s16 flying_cnt,landing_cnt;

extern s32 ref_height_get;

float stop_baro_hpf;

/*������*/

static s16 ld_delay_cnt ;
void land_discriminat(s16 dT_ms)
{
//	static s16 acc_delta,acc_old;
	
//	acc_delta = imu_data.w_acc[Z]- acc_old;
//	acc_old = imu_data.w_acc[Z];
	
	/*���Ź�һֵС��0.1  ���������Զ�����*/
	if((fs.speed_set_h_norm[Z] < 0.1f) || flag.auto_take_off_land == AUTO_LAND)
	{
		if(ld_delay_cnt>0)
		{
			ld_delay_cnt -= dT_ms;
		}
	}
	else
	{
		ld_delay_cnt = 200;
	}
	
	/*�����ǣ���������������ţ�����Ҫ�ȴ�ֱ������ٶ�С��200cm/s2 ����200ms�ſ�ʼ���*/	
	if(ld_delay_cnt <= 0 && (flag.thr_low || flag.auto_take_off_land == AUTO_LAND) )
	{
		/*�������������С��250����û�����ֶ��������������У�����1�룬��Ϊ��½��Ȼ������*/
		if(mc.ct_val_thr<250 && flag.unlock_sta == 1 && flag.locking != 2)//ABS(wz_spe_f1.out <20 ) //��Ӧ�� �����ٶ��������ٶ�С����20����ÿ�롣
		{
			if(landing_cnt<1500)
			{
				landing_cnt += dT_ms;
			}
			else
			{

				flying_cnt = 0;
				flag.taking_off = 0;
				///////
					landing_cnt =0;	
					flag.unlock_cmd =0;				

				flag.flying = 0;

			}
		}
		else
		{
			landing_cnt = 0;
		}
			
		
	}
	else
	{
		landing_cnt  = 0;
	}

}


/*����״̬����*/
int OF_pitspeed=0;
int OF_rolspeed=0;
void Flight_State_Task(u8 dT_ms,s16 *CH_N)
{
	s16 thr_deadzone;
	static float max_speed_lim,vel_z_tmp[2];
	/*��������ҡ����*/
	thr_deadzone = (flag.wifi_ch_en != 0) ? 0 : 50;
	fs.speed_set_h_norm[Z] = my_deadzone(CH_N[CH_THR],0,thr_deadzone) *0.0023f;
	fs.speed_set_h_norm_lpf[Z] += 0.5f *(fs.speed_set_h_norm[Z] - fs.speed_set_h_norm_lpf[Z]);
	
	/*���������*/
	if(flag.unlock_sta)
	{	
		if(fs.speed_set_h_norm[Z]>0.01f && flag.motor_preparation == 1) // 0-1
		{
			flag.taking_off = 1;
		}	
	}		
	//
	fc_stv.vel_limit_z_p = MAX_Z_SPEED_UP;
	fc_stv.vel_limit_z_n = -MAX_Z_SPEED_DW;	
	//
	if(flag.taking_off)
	{
			
		if(flying_cnt<1000)//800ms
		{
			flying_cnt += dT_ms;
		}
		else
		{
			/*��ɺ�1�룬��Ϊ�Ѿ��ڷ���*/
			flag.flying = 1;  
		}
		
		if(fs.speed_set_h_norm[Z]>0)
		{
			/*���������ٶ�*/
			vel_z_tmp[0] = (fs.speed_set_h_norm_lpf[Z] *MAX_Z_SPEED_UP);
		}
		else
		{
			/*�����½��ٶ�*/
			vel_z_tmp[0] = (fs.speed_set_h_norm_lpf[Z] *MAX_Z_SPEED_DW);
		}

		//�ɿ�ϵͳZ�ٶ�Ŀ�����ۺ��趨
		vel_z_tmp[1] = vel_z_tmp[0] + program_ctrl.vel_cmps_h[Z] + pc_user.vel_cmps_set_z;
		//
		vel_z_tmp[1] = LIMIT(vel_z_tmp[1],fc_stv.vel_limit_z_n,fc_stv.vel_limit_z_p);
		//
		fs.speed_set_h[Z] += LIMIT((vel_z_tmp[1] - fs.speed_set_h[Z]),-0.8f,0.8f);//������������
	}
	else
	{
		fs.speed_set_h[Z] = 0 ;
	}
	float speed_set_tmp[2];
	/*�ٶ��趨���������ο�ANO����ο�����*/
	fs.speed_set_h_norm[X] = (my_deadzone(+CH_N[CH_PIT],0,50) *0.0022f);
	fs.speed_set_h_norm[Y] = (my_deadzone(-CH_N[CH_ROL],0,50) *0.0022f);
		
	LPF_1_(3.0f,dT_ms*1e-3f,fs.speed_set_h_norm[X],fs.speed_set_h_norm_lpf[X]);
	LPF_1_(3.0f,dT_ms*1e-3f,fs.speed_set_h_norm[Y],fs.speed_set_h_norm_lpf[Y]);
	
	max_speed_lim = MAX_SPEED;
	
	if(switchs.of_flow_on && !switchs.gps_on )
	{
		max_speed_lim = 3.0f *wcz_hei_fus.out;
		max_speed_lim = LIMIT(max_speed_lim,50,200);
	}
	
	fc_stv.vel_limit_xy = max_speed_lim;
	
	//�ɿ�ϵͳXY�ٶ�Ŀ�����ۺ��趨
	speed_set_tmp[X] = fc_stv.vel_limit_xy *fs.speed_set_h_norm_lpf[X] + program_ctrl.vel_cmps_h[X] + pc_user.vel_cmps_set_h[X] + OF_pitspeed;
	speed_set_tmp[Y] = fc_stv.vel_limit_xy *fs.speed_set_h_norm_lpf[Y] + program_ctrl.vel_cmps_h[Y] + pc_user.vel_cmps_set_h[Y] + OF_rolspeed;
	
	length_limit(&speed_set_tmp[X],&speed_set_tmp[Y],fc_stv.vel_limit_xy,fs.speed_set_h_cms);

	fs.speed_set_h[X] = fs.speed_set_h_cms[X];
	fs.speed_set_h[Y] = fs.speed_set_h_cms[Y];	
	
	/*���ü����½�ĺ���*/
	land_discriminat(dT_ms);
	
	/*��б��������*/
	if(rolling_flag.rolling_step == ROLL_END)
	{
		if(imu_data.z_vec[Z]<0.25f)//75��  ////////////////////////////////////////*************************** ��б�������������á�
		{
			//
			if(mag.mag_CALIBRATE==0)
			{
				imu_state.G_reset = 1;
			}
			flag.unlock_cmd = 0;
		}

	}	
		//////////////////////////////////////////////////////////
	/*У׼�У���λ��������*/
	if(st_imu_cali.gyr_cali_on != 0 || st_imu_cali.acc_cali_on != 0)
	{
		if(flag.unlock_sta==0)
		{
			imu_state.G_reset = 1;
		}
	}
	
	/*��λ��������ʱ����Ϊ������ʧЧ*/
	if(imu_state.G_reset == 1)
	{
		flag.sensor_imu_ok = 0;
		LED_STA.rst_imu = 1;
		WCZ_Data_Reset(); //��λ�߶������ں�
	}
	else if(imu_state.G_reset == 0)
	{	
		if(flag.sensor_imu_ok == 0)
		{
			flag.sensor_imu_ok = 1;
			LED_STA.rst_imu = 0;
			AnoDTSendStr(USE_HID|USE_U2,SWJ_ADDR,LOG_COLOR_GREEN,"IMU OK!");
		}
	}
	
	/*����״̬��λ*/
	if(flag.unlock_sta == 0)
	{
		flag.flying = 0;
		landing_cnt = 0;
		flag.taking_off = 0;
		flying_cnt = 0;
		
		
		flag.rc_loss_back_home = 0;
		
		//��λ�ں�
		if(flag.taking_off == 0)
		{
//			wxyz_fusion_reset();
		}
	}
	

}

//
static u8 of_quality_ok;
static u16 of_quality_delay;
//
static u8 of_alt_ok;
static s16 of_alt_delay;
//
static u8 of_tof_on_tmp;
//

_judge_sync_data_st jsdata;

 


void Swtich_State_Task(u8 dT_ms)
{
	switchs.baro_on = 1;
	
	//
	sens_hd_check.of_ok = (ano_of.work_sta*ano_of.link_sta);
	//����ģ��
	if(sens_hd_check.of_ok || sens_hd_check.of_df_ok)
	{
		//
		if(sens_hd_check.of_ok)
		{
			jsdata.of_qua = ano_of.of_quality;
			jsdata.of_alt = (u16)ano_of.of_alt_cm;
		}
		else if(sens_hd_check.of_df_ok)
		{
			jsdata.of_qua = of_rdf.quality;
			jsdata.of_alt = Laser_height_cm;
		}
		
		//
		if(jsdata.of_qua>50 )//|| flag.flying == 0) //������������50 /*�����ڷ���֮ǰ*/����Ϊ�������ã��ж������ӳ�ʱ��Ϊ1��
		{
			if(of_quality_delay<500)
			{
				of_quality_delay += dT_ms;
			}
			else
			{
				of_quality_ok = 1;
			}
		}
		else
		{
			of_quality_delay =0;
			of_quality_ok = 0;
		}
		
		//�����߶�600cm����Ч
		if(jsdata.of_alt<600)
		{
			//		
			jsdata.valid_of_alt_cm = jsdata.of_alt;
			//��ʱ1.5���жϼ���߶��Ƿ���Ч
			if(of_alt_delay<1000)
			{
				of_alt_delay += dT_ms;			
			}
			else
			{
				//�ж��߶���Ч
				of_alt_ok = 1;
				of_tof_on_tmp = 1;
			}
		}
		else
		{
			//
			if(of_alt_delay>0)
			{
				of_alt_delay -= dT_ms;	
			}
			else
			{
				//�ж��߶���Ч
				of_alt_ok = 0;
				of_tof_on_tmp = 0;
			}				
		}
		//
		
		
		//
		if(flag.flight_mode == LOC_HOLD)
		{		
			if(of_alt_ok && of_quality_ok)
			{
				switchs.of_flow_on = 1;
			}
			else
			{
				switchs.of_flow_on = 0;
			}

		}
		else
		{
			of_tof_on_tmp = 0;
			switchs.of_flow_on = 0;
		}	
		//
		switchs.of_tof_on = of_tof_on_tmp;
	}
	else
	{
		switchs.of_flow_on = switchs.of_tof_on = 0;
	}
	
	
	
	


	//����ģ��
	if(sens_hd_check.tof_ok)
	{
		if(0)//(Laser_height_mm<1900)
		{
			switchs.tof_on = 1;
		}
		else
		{
			switchs.tof_on = 0;
		}
	}
	else
	{
		switchs.tof_on = 0;
	}
	
	//GPS	
	
	
	//UWB
	if(uwb_data.online && flag.flight_mode == LOC_HOLD)
	{
		switchs.uwb_on = 1;
	}
	else
	{
		switchs.uwb_on = 0;
	}
	
	
	//OPMV
	if(opmv.offline==0 && flag.flight_mode == LOC_HOLD)
	{
		switchs.opmv_on = 1;
	}
	else
	{
		switchs.opmv_on = 0;
	}
}


static void Speed_Mode_Switch()
{
//	
//	if( ubx_user_data.s_acc_cms > 60)// || ubx_user_data.svs_used < 6)
//	{
//		flag.speed_mode = 0;
//	}
//	else
//	{
//		flag.speed_mode = 1;
//	}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////



u8 speed_mode_old = 255;
u8 flight_mode_old = 255;
unsigned char* usrat_flag;

extern float  Distance;///����������
extern u16  cut2_flag;

void Flight_Mode_Set(u8 dT_ms)
{
	Speed_Mode_Switch();

	
	if(speed_mode_old != flag.speed_mode) //״̬�ı�
	{
		speed_mode_old = flag.speed_mode;
		//xy_speed_pid_init(flag.speed_mode);////////////
	}

///////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
	if(CH_N[AUX1]<-300)
	{
//		flag.flight_mode = ATT_STAB;////����̬ģʽ 
		flag.flight_mode = LOC_HOLD;
		
	}
							else if(CH_N[AUX1]<200)
							{
								
			//			//		flag.flight_mode = LOC_HOLD;// ���߶���ģʽ
			//					
			//						if(flag.unlock_sta == 1)
			//							{
			//								one_key_take_off();  //now:50cm
			//							}
								
											change_flag1 = 1;
							}
	else
	{
		
				flag.auto_take_off_land = AUTO_LAND;//һ������
//			flag.unlock_sta = 0;//��������
//		flag.flight_mode = RETURN_HOME;// gps����ģʽ
	}
	
	
	
	if(flight_mode_old != flag.flight_mode) //ҡ�˶�Ӧģʽ״̬�ı�
	{
		flight_mode_old = flag.flight_mode;
		
		flag.rc_loss_back_home = 0;

	}
	//
	if(rc_in.no_signal==0)//flag.rc_loss ==0)//���ջ����ź�
	{
		
		//CH_N[]+1500Ϊ��λ����ʾͨ��ֵ
		if(CH_N[AUX2]<-300)//<1200
		{
//			flag.flight_mode2 = 0;
//			pole_flag = 0;
			start_flag = 0;
		}
		else if(CH_N[AUX2]<200)//1200-1700
		{
			
					start_flag = 1;		//
			
//								start_flag = 2;		//

			
			
//						cut2_flag = 1;		 // ����28��	

			
//			flag.flight_mode2 = 3//׷����ɫ�飩				
//			flag.flight_mode2 = 1;//			
//					search_flag = 2;//1,Ѱ��  2,ǰ������ͷ׷��			
//						pole_flag = 1;  //�����ҽ����
			
			


		}
		else//>=1700
		{
			flag.unlock_sta = 0;//��������

//				pole_flag = 1;			
//				flag.auto_take_off_land = AUTO_LAND;//һ������			
//			flag.unlock_sta = 0;//��������
//			flag.flight_mode2 = 2;
		}
	}
	else
	{
		flag.flight_mode2 = 0;
	}
		
	
//////////////////////////////////////////////////////////////////	
	if(flag.auto_take_off_land == AUTO_LAND)
{
	if(ano_of.of_alt_cm<6)
	{
		flag.unlock_sta = 0;//��������

	}
}
////////////////////////////////////////////




//			if(Distance<=50)
//			{
//				OF_pitspeed=-20;
//			}	
//			else 
//			{
//				OF_pitspeed=0;	
//			}
//////////////////////////////////////////////////////////////	

}

////////////////////////////////////////////////////////////////////////








extern u8  TIM4CH1_CAPTURE_STA;		//���벶��״̬		    				
extern u32	TIM4CH1_CAPTURE_VAL;	//���벶��ֵ  
long long temp=0; 
float  Distance;	

void ultrasonic()
{

	
	GPIO_SetBits(GPIOD, GPIO_Pin_5);   //����pf9��ƽ	
	Delay_us(30);//����30us	
	GPIO_ResetBits(GPIOD, GPIO_Pin_5); //����pf9��ƽ
		
				if(TIM4CH1_CAPTURE_STA&0X80)        //�ɹ�������һ�θߵ�ƽ
				{
					
					temp=TIM4CH1_CAPTURE_STA&0X3F; 
					temp*=0XFFFFFFFF;		 		         //���ʱ���ܺ�
					temp+=TIM4CH1_CAPTURE_VAL;		   //�õ��ܵĸߵ�ƽʱ��
					Distance=temp*340/20000;
					TIM4CH1_CAPTURE_STA=0;			     //������һ�β���
				}
	

		
}

//////////////////////////////////////////////////////////////////////////////////////
u16 OF_flag=0;
u16  turnstile_flag_left=0;
u16  turnstile_flag_right=0;
//int OF_pitspeed=0;
//int OF_rolspeed=0;
#include "Ano_OPMV_LineTracking_Ctrl.h"

void flying_bling(u8 dT_ms)
{
		if(	flag.flight_mode2 == 1)
		{
			
			OF_flag++;
			if(OF_flag<500)
			{
				OF_pitspeed=10;
			}
			else if((OF_flag>=500)&&(OF_flag<1000))
			{
				OF_rolspeed=10;
				OF_pitspeed=0;
			}
			
			else if((OF_flag>=1000)&&(OF_flag<1500))
			{
				OF_pitspeed=-10;
				OF_rolspeed=0;
			}
			else if((OF_flag>=1500)&&(OF_flag<2000))
			{
				OF_rolspeed=-10;
				OF_pitspeed=0;
			}
			
			else
			{
				OF_pitspeed=0;
				OF_rolspeed=0;
			flag.auto_take_off_land = AUTO_LAND;//һ������


			}
		}
}


///////////////////////////////////////////////////////////////////////


extern float artificial_yaw;
//extern _mc_st mc;

void  turnstile(u8 dT_ms)
{

	
		if(	flag.flight_mode2 == 1)
{
//			search_flag=0;
			
//					turnstile_flag1++;
			
					if(opmv.user.data0==0)
					{
						
						
						program_ctrl.yaw_pal_dps= 10;///��Ϊ��ת����Ϊ��ת����ʱ�룩

						OF_rolspeed=0;////rollΪ���ҷɷ�֮Ϊ���

					}
									else if((opmv.user.data0>=10)&&(opmv.user.data0<=130))
									 {
										 				program_ctrl.yaw_pal_dps= 45;///��Ϊ��ת����Ϊ��ת����ʱ�룩


									 }
										 
												else if((opmv.user.data0>130)&&(opmv.user.data0<150))
										 {
									///////			
																													turnstile_flag_right++;										
																												if(turnstile_flag_right<500)
																												{	
																											
																												OF_rolspeed=-15;////rollΪ���ҷɷ�֮Ϊ���
																													

																												}
																												else if((turnstile_flag_right>=500)&&(turnstile_flag_right<750))
																												{
																													
																													 OF_pitspeed=-15;

																													 OF_rolspeed=0;
																												}
							
																										else
																										{
																											OF_pitspeed=0;
																											OF_rolspeed=0;
																											
													


//																									flag.auto_take_off_land = AUTO_LAND;//һ������
																											
																											
																											pole_flag=1;//�л�Ѱ��
																										flag.flight_mode2=0;
																											
																										}
											//////////////////////////////
//																			if(opmv.user.data1>45)	
//															{
//															OF_pitspeed=10;
//															}
//																							else if(opmv.user.data1<35)
//																							{
//																								
//																								OF_pitspeed=-20;
//										
//																							}
//																				else if((opmv.user.data1>=35)&&(opmv.user.data1<=45))
//																				{
//																					
//																				}
//																				
//																					else
//																						
//																					{
//																							
//																					}
//																					
												
																
							}

//							
		
													
								 
												else if (opmv.user.data0>150)
																{
																	
																			program_ctrl.yaw_pal_dps= -45;///��Ϊ��ת����Ϊ��ת����ʱ�룩

//																				artificial_yaw += -1.0f;////yawΪ����˳ʱ�룬��֮��ʱ��
//																	OF_rolspeed=-15;////rollΪ���ҷɷ�֮Ϊ���
																}
																
																
																				else
																				{
																				
																				}
								
																					


	///////																			
							if(turnstile_flag_right<500)			
							{
			
															if(opmv.user.data1>50)	
															{
															OF_pitspeed=25;
															}
																							else if(opmv.user.data1<40)
																							{
																								
																								OF_pitspeed=-15;
										
																							}
																				else if((opmv.user.data1>=40)&&(opmv.user.data1<=50))
																				{
																					
																				}
																				
																					else
																						
																					{
																							
																					}
																					
																																													
					}	

					else if((turnstile_flag_right>=500)&&(turnstile_flag_right<750))
					{
													if(opmv.user.data1>70)	
															{
															OF_pitspeed=15;
															}
																							else if(opmv.user.data1<60)
																							{
																								
																								OF_pitspeed=-15;
											
																							}
																				else if((opmv.user.data1>=40)&&(opmv.user.data1<=50))
																				{
																					
																				}
																				
																					else
																						
																					{
																							
																					}
					}

					else
					{
						
					}
/////						
																						
}
		else
		{
				turnstile_flag_right=0;

		}

		
		
/////////////////////////////////////////////////////////////////////////////		
//////////////////////////////////////////////////////////////////////////////		
		
		
		
		
		
		if(flag.flight_mode2 == 2)
		{
	       
			
								if(opmv.user.data0==0)
					{
							program_ctrl.yaw_pal_dps= 10;///��Ϊ��ת����Ϊ��ת����ʱ�룩
//						artificial_yaw += 0.3f;////yawΪ����˳ʱ�� ��֮��ʱ��
						OF_rolspeed=0;////rollΪ���ҷɷ�֮Ϊ���

					}
									else if((opmv.user.data0>=10)&&(opmv.user.data0<=130))
									 {
										 
										 							program_ctrl.yaw_pal_dps= -45;///��Ϊ��ת����Ϊ��ת����ʱ�룩

//													artificial_yaw += -1.0f;////yawΪ����˳ʱ�� ��֮��ʱ��
//										 		OF_rolspeed=15;////rollΪ���ҷɷ�֮Ϊ���


									 }
										 
												else if((opmv.user.data0>130)&&(opmv.user.data0<150))
										 {
									///////			
																													turnstile_flag_left++;										
																												if(turnstile_flag_left<500)
																												{	

																											
																//												artificial_yaw += 0.8;////yawΪ����˳ʱ�� ��֮��ʱ��
																												OF_rolspeed=15;////rollΪ���ҷɷ�֮Ϊ���
																													

																												}
																												else if((turnstile_flag_left>=500)&&(turnstile_flag_left<750))
																												{
																													
																													 OF_pitspeed=-10;

																													 OF_rolspeed=0;
																							//						artificial_yaw = 0;////
																												}
																					//					
									
																										else
																										{
																											OF_pitspeed=0;
																											OF_rolspeed=0;
																											
																											pole_flag=2;//�л�Ѱ��
//																										flag.flight_mode2=0;
																											
																										}
											//////////////////////////////

							}

//							
		
													
								 
												else if (opmv.user.data0>150)
																{
																	
																		program_ctrl.yaw_pal_dps= 45;///��Ϊ��ת����Ϊ��ת����ʱ�룩

																}
																
																
																				else
																				{
																				
																				}
								
																					

	///////																			
			if(turnstile_flag_left<500)			
			{
			
															if(opmv.user.data1>50)	
															{
															OF_pitspeed=25;
															}
																							else if(opmv.user.data1<40)
																							{
																								
																								OF_pitspeed=-15;
										
																							}
																				else if((opmv.user.data1>=40)&&(opmv.user.data1<=50))
																				{
																					
																				}
																				
																					else
																						
																					{
																							
																					}
																					
																																													
				}	
			
									else if((turnstile_flag_left>=500)&&(turnstile_flag_left<750))
					{
													if(opmv.user.data1>70)	
															{
															OF_pitspeed=15;
															}
																							else if(opmv.user.data1<60)
																							{
																								
																								OF_pitspeed=-15;
											
																							}
																				else if((opmv.user.data1>=40)&&(opmv.user.data1<=50))
																				{
																					
																				}
																				
																					else
																						
																					{
																							
																					}
					}

					else
					{
						
					}
					
//////////////////////////////////////////////////////////



			
			
		}
		else
		{
				turnstile_flag_left=0;

		}
		
		
	///////////////////////////////////////////////////////////	/
		
									if(pole_flag==2)
							{
														flag.flight_mode2 = 3;
								
													
																if(((20<=opmv.cb.pos_x)&&(opmv.cb.pos_x<40))&&((-20<=opmv.cb.pos_y)&&(opmv.cb.pos_y<-5)))
																{
																
																	flag.auto_take_off_land = AUTO_LAND;//һ������
//																	flag.flight_mode2=0;
																
																}
													
							}
		
}




////////////////////////////////////////////////////////////////////
u16 	bar_flag=0;

void search_bar(u8 dT_ms)
{
	
	
//	if(pole_flag == 0 )
//	{
//			static u8 sta = 1;																		
//			usart_flag= sta;
//			uasrt2_senddata(sta);

//	}
	
		
	if(pole_flag == 1)
	{
			static u8 sta = 5;																		
			usart_flag= sta;
			uasrt2_senddata(sta);

	}
	
	
				if(( search_flag== 1)&&(flag.flight_mode2==0))	
				{

					
					if(opmv.user.data0==0)					
					{
							program_ctrl.yaw_pal_dps= -10;///��Ϊ��ת����Ϊ��ת����ʱ�룩
//						artificial_yaw += -0.3f;////yawΪ����˳ʱ�� ��֮��ʱ��
					}
						
							else if((opmv.user.data0>=10)&&(opmv.user.data0<=130))
							{
//									artificial_yaw += 0.1f;////yawΪ����˳ʱ�� ��֮��ʱ��
									 program_ctrl.yaw_pal_dps= 8;///��Ϊ��ת����Ϊ��ת����ʱ�룩

							}
							
							
										else if((opmv.user.data0>130)&&(opmv.user.data0<150))
										{
											

																				if(opmv.user.data1>50)	
																				{
																								OF_pitspeed=  8 ;
																				}
																				else if(opmv.user.data1<40)
																				{
																				
																								OF_pitspeed=-10;
																					
																			
																				}
																				else if((opmv.user.data1>=40)&&(opmv.user.data1<=50))
																				{
																					
																					if(pole_flag==0)
																					{	
																						
																					flag.flight_mode2 = 1;
																						
																					}
																					else if(pole_flag==1)
																					{
																						
																					flag.flight_mode2 = 2;
																						
																					}
																					else
																					{
																					
																					}
																				
																				}
																				else
																					
																				{
																								OF_pitspeed=0;
																				}
																					
															
																					
/////////////////////////////////////////////////////////////////////////////////////////
										
										}
					
																else if (opmv.user.data0>150)
																{
//																	artificial_yaw += -0.1f;////yawΪ����˳ʱ�룬��֮��ʱ��
																		program_ctrl.yaw_pal_dps= -8;///��Ϊ��ת����Ϊ��ת����ʱ�룩

																}
																
																
																				else
																				{
																				
																				}
								
					
				
				}
			

}


void control(u8 dT_ms)
	
{
				if( search_flag == 2)
				{

					
					if(opmv.user.data0==0)					
					{
							program_ctrl.yaw_pal_dps= -10;///��Ϊ��ת����Ϊ��ת����ʱ�룩
//						artificial_yaw += -0.3f;////yawΪ����˳ʱ�� ��֮��ʱ��
					}
						
							else if((opmv.user.data0>=10)&&(opmv.user.data0<=130))
							{
//									artificial_yaw += 0.1f;////yawΪ����˳ʱ�� ��֮��ʱ��
//									 program_ctrl.yaw_pal_dps= 8;///��Ϊ��ת����Ϊ��ת����ʱ�룩
								
											OF_rolspeed=5;	////rollΪ���ҷɷ�֮Ϊ���



							}
							
							
										else if((opmv.user.data0>130)&&(opmv.user.data0<150))
										{
											
																					OF_pitspeed = 0 ;
																					OF_rolspeed = 0 ; 

										
										}
					
																else if (opmv.user.data0>150)
																{
//																	artificial_yaw += -0.1f;////yawΪ����˳ʱ�룬��֮��ʱ��
//																		program_ctrl.yaw_pal_dps= -8;///��Ϊ��ת����Ϊ��ת����ʱ�룩
																		OF_rolspeed = -5;	////rollΪ���ҷɷ�֮Ϊ���
																}
																
																										else if((opmv.user.data0>110)&&(opmv.user.data0<170))
																										{
																											
																																		if(opmv.user.data1>60)	
																																			
																																			{
																																							OF_pitspeed=  5 ;
																																			}
																																			
																																			else if(opmv.user.data1<50)
																																			{
																																							OF_pitspeed= -5;
																																			}
																																			else if((opmv.user.data1>=50)&&(opmv.user.data1<=60))
																																				{
																																					OF_pitspeed = 0 ;
																																					OF_rolspeed = 0 ; 
																																			}
																																			else
																																			{
																																							OF_pitspeed=0;
																																			}
																																				
																										}
																										
																										
																
																				else
																				{
																				
																				}
																				

									}

}




