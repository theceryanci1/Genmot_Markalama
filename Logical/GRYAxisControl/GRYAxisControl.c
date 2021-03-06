/*******************************************************************
 * COPYRIGHT -- Bernecker + Rainer
 ********************************************************************
 * PROGRAM: Basic
 * File: Basic.c
 * Author: Bernecker + Rainer
 * Created: December 01, 2009
 ********************************************************************
 * Implementation of Program Basic
 ********************************************************************/

#ifdef _DEFAULT_INCLUDES
#include <AsDefault.h>
#endif
#include <string.h>

/* defines of the state-constants */
#define STATE_WAIT              0  
#define STATE_POWER_ON          1  
#define STATE_HOME              2

#define STATE_READY             10
#define STATE_HALT              11
#define STATE_STOP              12
#define STATE_JOG_POSITIVE      13
#define STATE_JOG_NEGATIVE      14
#define STATE_MOVE_ABSOLUTE     15
#define STATE_MOVE_ADDITIVE     16
#define STATE_MOVE_VELOCITY     17

#define STATE_ERROR_AXIS        100
#define STATE_ERROR             101
#define STATE_ERROR_RESET       102
#define STATE_READ              103

_LOCAL REAL   data;


_INIT void GRYAxis_init(void)
{

	/* get axis object */
	Axis1Obj = (UDINT)&GRY_Axis;

	AxisStep = STATE_WAIT;  /* start step */

	GRYAxis.Parameter.Velocity              = 10000;  /*velocity for movement*/
	GRYAxis.Parameter.Acceleration          = 40000;  /*acceleration for movement*/
	GRYAxis.Parameter.Deceleration          = 40000;  /*deceleration for movement*/
	GRYAxis.Parameter.JogVelocity           = 10000;   /*velocity for jogging */

}

_CYCLIC void GRYAxis_cyclic(void)
{
	if(GRYAxis.Command.Power)
	{
		GRYAxis.Speed.VelocityOverride			= 1.0;
	}
	
/***************************************************************
        Control Sequence
***************************************************************/

/* status information is read before the step sequencer to attain a shorter reaction time */
/************************ MC_READSTATUS *************************/
	MC_ReadStatus_0.Enable = !MC_ReadStatus_0.Error;
	MC_ReadStatus_0.Axis = Axis1Obj;
	MC_ReadStatus(&MC_ReadStatus_0);
	GRYAxis.AxisState.Disabled             = MC_ReadStatus_0.Disabled;
	GRYAxis.AxisState.StandStill           = MC_ReadStatus_0.StandStill;
	GRYAxis.AxisState.Stopping             = MC_ReadStatus_0.Stopping;
	GRYAxis.AxisState.Homing               = MC_ReadStatus_0.Homing;
	GRYAxis.AxisState.DiscreteMotion       = MC_ReadStatus_0.DiscreteMotion;
	GRYAxis.AxisState.ContinuousMotion     = MC_ReadStatus_0.ContinuousMotion;
	GRYAxis.AxisState.SynchronizedMotion   = MC_ReadStatus_0.SynchronizedMotion;
	GRYAxis.AxisState.ErrorStop            = MC_ReadStatus_0.Errorstop;

	/********************MC_BR_READDRIVESTATUS***********************/
	MC_BR_ReadDriveStatus_0.Enable = !MC_BR_ReadDriveStatus_0.Error;
	MC_BR_ReadDriveStatus_0.Axis = Axis1Obj; 
	MC_BR_ReadDriveStatus_0.AdrDriveStatus = (UDINT)&(GRYAxis.Status.DriveStatus);
	MC_BR_ReadDriveStatus(&MC_BR_ReadDriveStatus_0);

	/******************** MC_READACTUALPOSITION *********************/
	MC_ReadActualPosition_0.Enable = (!MC_ReadActualPosition_0.Error);
	MC_ReadActualPosition_0.Axis = Axis1Obj;
	MC_ReadActualPosition(&MC_ReadActualPosition_0);
	if(MC_ReadActualPosition_0.Valid == 1)
	{
		GRYAxis.Status.ActPosition = MC_ReadActualPosition_0.Position;
	}

	/******************** MC_READACTUALVELOCITY *********************/
	MC_ReadActualVelocity_0.Enable = (!MC_ReadActualVelocity_0.Error);
	MC_ReadActualVelocity_0.Axis = Axis1Obj;
	MC_ReadActualVelocity(&MC_ReadActualVelocity_0);
	if(MC_ReadActualVelocity_0.Valid == 1)
	{
		GRYAxis.Status.ActVelocity = MC_ReadActualVelocity_0.Velocity;
	}
	
	/*********************** MC SET OVERRIDE *********************/
	MC_SetOverride_0.Enable = (!MC_SetOverride_0.Error);
	MC_SetOverride_0.AccFactor = GRYAxis.Speed.AccelerationOverride = 1.0;
	if ((GRYAxis.Speed.VelocityOverride < 2.01) && (GRYAxis.Speed.VelocityOverride > 0.0000001))
	{
		MC_SetOverride_0.VelFactor = GRYAxis.Speed.VelocityOverride;
	}
	MC_SetOverride_0.Axis=Axis1Obj;
	MC_SetOverride(&MC_SetOverride_0);
	

	/************************ MC_READAXISERROR **********************/
	MC_ReadAxisError_0.Enable = !MC_ReadAxisError_0.Error;
	MC_ReadAxisError_0.Axis = Axis1Obj;
	MC_ReadAxisError_0.DataAddress = (UDINT)&(GRYAxis.Status.ErrorText);
	MC_ReadAxisError_0.DataLength = sizeof(GRYAxis.Status.ErrorText);
	strcpy((void*)&MC_ReadAxisError_0.DataObjectName,"acp10etxen");
	MC_ReadAxisError(&MC_ReadAxisError_0);

	/* central monitoring of stop command attains a shorter reaction time in case of emergency stop */
	/******************CHECK for STOP COMMAND************************/
	if (GRYAxis.Command.Stop == 1)
	{
		if ((AxisStep >= STATE_READY) && (AxisStep < STATE_ERROR))
		{
			/* reset all FB execute inputs we use */
			MC_Home_0.Execute = 0;
			MC_Stop_0.Execute = 0;
			MC_MoveAbsolute_0.Execute = 0;
			MC_MoveAdditive_0.Execute = 0;
			MC_MoveVelocity_0.Execute = 0;
			MC_ReadAxisError_0.Acknowledge = 0;
			MC_Reset_0.Execute = 0;

			/* reset user commands */
			GRYAxis.Command.Stop = 0;
			GRYAxis.Command.Home = 0;
			GRYAxis.Command.MoveJogPos = 0;
			GRYAxis.Command.MoveJogNeg = 0;
			GRYAxis.Command.MoveVelocity = 0;
			GRYAxis.Command.MoveAbsolute = 0;
			GRYAxis.Command.MoveAdditive = 0;
			AxisStep = STATE_STOP;
		}
	}

	/**************** CHECK FOR GENERAL AXIS ERROR ******************/
	if ((MC_ReadAxisError_0.AxisErrorID != 0) && (MC_ReadAxisError_0.Valid == 1))
	{
		AxisStep = STATE_ERROR_AXIS;
	}
		/***************** CHECK IF POWER SHOULD BE OFF *******************/
	else if ((GRYAxis.Command.Power == 0) && (MC_ReadAxisError_0.Valid == 1))
	{
		if ((MC_ReadStatus_0.Errorstop == 1) && (MC_ReadStatus_0.Valid == 1))
		{
			AxisStep = STATE_ERROR_RESET;
		}
		else
		{
			AxisStep = STATE_WAIT;
		}
	}

	switch(AxisStep)
	{
		/******************** WAIT *************************/
		case STATE_WAIT:  /* STATE: Wait */
			if (GRYAxis.Command.Power == 1)
			{
				AxisStep = STATE_POWER_ON;
			}
			else
			{
				MC_Power_0.Enable = 0;
				
			}

			/* reset all FB execute inputs we use */
			MC_Home_0.Execute = 0;
			MC_Stop_0.Execute = 0;
			MC_MoveAbsolute_0.Execute = 0;
			MC_MoveAdditive_0.Execute = 0;
			MC_MoveVelocity_0.Execute = 0;
			MC_ReadAxisError_0.Acknowledge = 0;
			MC_Reset_0.Execute = 0;

			/* reset user commands */
			GRYAxis.Command.Stop = 0;
			GRYAxis.Command.Home = 0;
			GRYAxis.Command.MoveJogPos = 0;
			GRYAxis.Command.MoveJogNeg = 0;
			GRYAxis.Command.MoveVelocity = 0;
			GRYAxis.Command.MoveAbsolute = 0;
			GRYAxis.Command.MoveAdditive = 0;

			GRYAxis.Status.ErrorID = 0;
			break;

		/******************** POWER ON **********************/
		case STATE_POWER_ON:  /* STATE: Power on */
			MC_Power_0.Enable = 1;
			if (MC_Power_0.Status == 1)
			{
			if (!GRXHomeOk)
			{
			GRYAxis.Command.Home = 1;
			}
			AxisStep = STATE_READY;
			}
			/* if a power error occured go to error state */
			if (MC_Power_0.Error == 1)
			{
				GRYAxis.Status.ErrorID = MC_Power_0.ErrorID;
				AxisStep = STATE_ERROR;
			}
			break;

		/******************** READY **********************/
		case STATE_READY:  /* STATE: Waiting for commands */
			if (GRYAxis.Command.Home == 1)
			{
				GRYAxis.Command.Home = 0;
				AxisStep = STATE_HOME;
			}
			else if (GRYAxis.Command.Stop == 1)
			{
				AxisStep = STATE_STOP;
			}
			else if (GRYAxis.Command.MoveJogPos == 1)
			{
				AxisStep = STATE_JOG_POSITIVE;
			}
			else if (GRYAxis.Command.MoveJogNeg == 1)
			{
				AxisStep = STATE_JOG_NEGATIVE;
			}
			else if (GRYAxis.Command.MoveAbsolute == 1)
			{
				GRYAxis.Command.MoveAbsolute = 0;
				AxisStep = STATE_MOVE_ABSOLUTE;
			}
			else if (GRYAxis.Command.MoveAdditive == 1)
			{
				GRYAxis.Command.MoveAdditive = 0;
				AxisStep = STATE_MOVE_ADDITIVE;
			}
			else if (GRYAxis.Command.MoveVelocity == 1)
			{
				GRYAxis.Command.MoveVelocity = 0;
				AxisStep = STATE_MOVE_VELOCITY;
			}
			else if (GRYAxis.Command.Halt == 1)
			{
				GRYAxis.Command.Halt = 0;
				AxisStep = STATE_HALT;   
			}
			break;
			

		/******************** HOME **********************/
		case STATE_HOME:  /* STATE: start homing process */
			MC_Home_0.Position = GRYAxis.Parameter.HomePosition;
			MC_Home_0.HomingMode = GRYAxis.Parameter.HomeMode;
			MC_Home_0.Execute = 1;
			if (MC_Home_0.Done == 1)
			{
				
				MC_Home_0.Execute = 0;
				AxisStep = STATE_READ;
			}
			/* if a homing error occured go to error state */
			if (MC_Home_0.Error == 1)
			{
				MC_Home_0.Execute = 0;
				GRYAxis.Status.ErrorID = MC_Home_0.ErrorID;
				AxisStep = STATE_ERROR;
			}
			break;
    
		/************************HALT MOVEMENT**************************/    
		case STATE_HALT:  /* STATE: Stop the active movement */
			MC_Halt_0.Deceleration = GRYAxis.Parameter.Deceleration;
			MC_Halt_0.Execute = 1;
			if (MC_Halt_0.Done == 1)
			{
				MC_Halt_0.Execute = 0;
				AxisStep = STATE_READY;  
			}
			/*check if error occured */
			if (MC_Halt_0.Error == 1)
			{
				MC_Halt_0.Execute = 0;
				GRYAxis.Status.ErrorID = MC_Halt_0.ErrorID;
				AxisStep = STATE_ERROR;   
			}
			break;

		/*********************** STOP MOVEMENT *************************/
		case STATE_STOP: /* STATE: Stop movement as long as command is set */
			MC_Stop_0.Deceleration = GRYAxis.Parameter.Deceleration;
			MC_Stop_0.Execute = 1;
			/* if axis is stopped go to ready state */
			if ((MC_Stop_0.Done == 1) && (GRYAxis.Command.Stop == 0))
			{
				MC_Stop_0.Execute = 0;
				AxisStep = STATE_READY;
			}
			/* check if error occured */
			if (MC_Stop_0.Error == 1)
			{
				GRYAxis.Status.ErrorID = MC_Stop_0.ErrorID;
				MC_Stop_0.Execute = 0;
				AxisStep = STATE_ERROR;
			}
			break;

		/******************** START JOG MOVEMENT POSITVE **********************/
		case STATE_JOG_POSITIVE:  /* STATE: Start jog movement in positive direction */
			MC_MoveVelocity_0.Velocity      = GRYAxis.Parameter.JogVelocity;
			MC_MoveVelocity_0.Acceleration  = GRYAxis.Parameter.Acceleration;
			MC_MoveVelocity_0.Deceleration  = GRYAxis.Parameter.Deceleration;
			MC_MoveVelocity_0.Direction     = mcPOSITIVE_DIR;
			MC_MoveVelocity_0.Execute = 1;
			/* check if jog movement should be stopped */
			if (GRYAxis.Command.MoveJogPos == 0)
			{
				MC_MoveVelocity_0.Execute = 0;
				AxisStep = STATE_HALT;
			}
			/* check if error occured */
			if (MC_MoveVelocity_0.Error == 1)
			{
				GRYAxis.Status.ErrorID = MC_MoveVelocity_0.ErrorID;
				MC_MoveVelocity_0.Execute = 0;
				AxisStep = STATE_ERROR;
			}
			break;

		/******************** START JOG MOVEMENT NEGATIVE **********************/
		case STATE_JOG_NEGATIVE:  /* STATE: Start jog movement in negative direction */
			MC_MoveVelocity_0.Velocity      = GRYAxis.Parameter.JogVelocity;
			MC_MoveVelocity_0.Acceleration  = GRYAxis.Parameter.Acceleration;
			MC_MoveVelocity_0.Deceleration  = GRYAxis.Parameter.Deceleration;
			MC_MoveVelocity_0.Direction     = mcNEGATIVE_DIR;
			MC_MoveVelocity_0.Execute = 1;        
			/* check if jog movement should be stopped */
			if (GRYAxis.Command.MoveJogNeg == 0)
			{
				MC_MoveVelocity_0.Execute = 0;
				AxisStep = STATE_HALT;
			}
			/* check if error occured */
			if (MC_MoveVelocity_0.Error == 1)
			{
				GRYAxis.Status.ErrorID = MC_MoveVelocity_0.ErrorID;
				MC_MoveVelocity_0.Execute = 0;
				AxisStep = STATE_ERROR;
			}
			break;

		/******************** START ABSOLUTE MOVEMENT **********************/
		case STATE_MOVE_ABSOLUTE:  /* STATE: Start absolute movement */
			MC_MoveAbsolute_0.Position      = GRYAxis.Parameter.Position;
			MC_MoveAbsolute_0.Velocity      = GRYAxis.Parameter.Velocity;
			MC_MoveAbsolute_0.Acceleration  = GRYAxis.Parameter.Acceleration;
			MC_MoveAbsolute_0.Deceleration  = GRYAxis.Parameter.Deceleration;
			MC_MoveAbsolute_0.Direction     = GRYAxis.Parameter.Direction;
			MC_MoveAbsolute_0.Execute = 1;
			/* check if commanded position is reached */
			if (GRYAxis.Command.Halt == 1)
			{
				GRYAxis.Command.Halt = 0;
				MC_MoveAbsolute_0.Execute = 0;
				AxisStep = STATE_HALT;   
			}
			else if (MC_MoveAbsolute_0.Done == 1)
			{
				MC_MoveAbsolute_0.Execute = 0;
				AxisStep = STATE_READY;
			}
			/* check if error occured */
			if (MC_MoveAbsolute_0.Error == 1)
			{
				GRYAxis.Status.ErrorID = MC_MoveAbsolute_0.ErrorID;
				MC_MoveAbsolute_0.Execute = 0;
				AxisStep = STATE_ERROR;
			}
			break;

		/******************** START ADDITIVE MOVEMENT **********************/
		case STATE_MOVE_ADDITIVE:  /* STATE: Start additive movement */
			MC_MoveAdditive_0.Distance      = GRYAxis.Parameter.Distance;
			MC_MoveAdditive_0.Velocity      = GRYAxis.Parameter.Velocity;
			MC_MoveAdditive_0.Acceleration  = GRYAxis.Parameter.Acceleration;
			MC_MoveAdditive_0.Deceleration  = GRYAxis.Parameter.Deceleration;
			MC_MoveAdditive_0.Execute = 1;
			/* check if commanded distance is reached */
			if (GRYAxis.Command.Halt == 1)
			{
				GRYAxis.Command.Halt = 0;
				MC_MoveAdditive_0.Execute = 0;
				AxisStep = STATE_HALT;   
			}
			else if (MC_MoveAdditive_0.Done == 1)
			{
				MC_MoveAdditive_0.Execute = 0;
				AxisStep = STATE_READY;
			}
			/* check if error occured */
			if (MC_MoveAdditive_0.Error == 1)
			{
				GRYAxis.Status.ErrorID = MC_MoveAdditive_0.ErrorID;
				MC_MoveAdditive_0.Execute = 0;
				AxisStep = STATE_ERROR;
			}
			break;

		/******************** START VELOCITY MOVEMENT **********************/
		case STATE_MOVE_VELOCITY:  /* STATE: Start velocity movement */
			MC_MoveVelocity_0.Velocity      = GRYAxis.Parameter.Velocity;
			MC_MoveVelocity_0.Acceleration  = GRYAxis.Parameter.Acceleration;
			MC_MoveVelocity_0.Deceleration  = GRYAxis.Parameter.Deceleration;
			MC_MoveVelocity_0.Direction     = GRYAxis.Parameter.Direction;
			MC_MoveVelocity_0.Execute = 1;
			/* check if commanded velocity is reached */
			if (GRYAxis.Command.Halt == 1)
			{
				GRYAxis.Command.Halt = 0;
				MC_MoveVelocity_0.Execute = 0;
				AxisStep = STATE_HALT;    
			}
			else if (MC_MoveVelocity_0.InVelocity == 1)
			{
				MC_MoveVelocity_0.Execute = 0;
				AxisStep = STATE_READY;
			}
			/* check if error occured */
			if (MC_MoveVelocity_0.Error == 1)
			{
				GRYAxis.Status.ErrorID = MC_MoveVelocity_0.ErrorID;
				MC_MoveVelocity_0.Execute = 0;
				AxisStep = STATE_ERROR;
			}
			break;

		/******************** FB-ERROR OCCURED *************************/
		case STATE_ERROR:  /* STATE: Error */
			/* check if FB indicates an axis error */
			if (MC_ReadAxisError_0.AxisErrorCount != 0)
			{
				AxisStep = STATE_ERROR_AXIS;
			}
			else
			{
				if (GRYAxis.Command.ErrorAcknowledge == 1)
				{
					GRYAxis.Command.ErrorAcknowledge = 0;
					GRYAxis.Status.ErrorID = 0;
					/* reset axis if it is in axis state ErrorStop */
					if ((MC_ReadStatus_0.Errorstop == 1) && (MC_ReadStatus_0.Valid == 1))
					{
						AxisStep = STATE_ERROR_RESET;
					}
					else
					{
						AxisStep = STATE_WAIT;
					}
				}
			}
			break;

		/******************** AXIS-ERROR OCCURED *************************/
		case STATE_ERROR_AXIS:  /* STATE: Axis Error */
			if (MC_ReadAxisError_0.Valid == 1)
			{
				if (MC_ReadAxisError_0.AxisErrorID != 0)
				{
					GRYAxis.Status.ErrorID = MC_ReadAxisError_0.AxisErrorID;
				}
				MC_ReadAxisError_0.Acknowledge = 0;
				if (GRYAxis.Command.ErrorAcknowledge == 1)
				{
					GRYAxis.Command.ErrorAcknowledge = 0;
					/* acknowledge axis error */
					if (MC_ReadAxisError_0.AxisErrorID != 0)
					{
						MC_ReadAxisError_0.Acknowledge = 1;
					}
				}
				if (MC_ReadAxisError_0.AxisErrorCount == 0)
				{
					GRYAxis.Status.ErrorID = 0;
					/* reset axis if it is in axis state ErrorStop */
					if ((MC_ReadStatus_0.Errorstop == 1) && (MC_ReadStatus_0.Valid == 1))
					{
						AxisStep = STATE_ERROR_RESET;
					}
					else
					{
						AxisStep = STATE_WAIT;
					}
				}
			}
			break;

		/******************** RESET DONE *************************/
		case STATE_ERROR_RESET:  /* STATE: Wait for reset done */
			MC_Reset_0.Execute = 1;
			/* reset MC_Power.Enable if this FB is in Error*/
			if(MC_Power_0.Error == 1)
			{
				MC_Power_0.Enable = 0;
			}
			if(MC_Reset_0.Done == 1)
			{
				MC_Reset_0.Execute = 0;
				AxisStep = STATE_WAIT;
			}
			else if (MC_Reset_0.Error == 1)
			{
				MC_Reset_0.Execute = 0;
				AxisStep = STATE_ERROR;   
			}
			break;
    
		/******************** SEQUENCE END *************************/
		
		/**********************STATE_READ************************/    
		case STATE_READ:
			MC_BR_CyclicRead_0.Enable=1;
			MC_BR_CyclicRead_0.ParID = 277;
			MC_BR_CyclicRead_0.DataAddress = (UDINT)&data;
			MC_BR_CyclicRead_0.DataType =ncPAR_TYP_REAL;
      
			if (MC_BR_CyclicRead_0.Valid) 
			{
				AxisStep = STATE_READY;
			}
			else if (MC_BR_CyclicRead_0.Error)
			{

				GRYAxis.Status.ErrorID = MC_BR_CyclicRead_0.ErrorID;
				/*Reset Enable before going to STATE_ERROR*/
				MC_BR_CyclicRead_0.Enable=0;
				AxisStep = STATE_ERROR;
			}
			break;  /*end read*/
   

	}

/***************************************************************
        Function Block Calls
***************************************************************/

/************************** MC_POWER ****************************/
	MC_Power_0.Axis = Axis1Obj;  /* pointer to axis */
	MC_Power(&MC_Power_0);

	/************************** MC_HOME *****************************/
	MC_Home_0.Axis = Axis1Obj;
	MC_Home(&MC_Home_0);

	/********************** MC_MOVEABSOLUTE *************************/
	MC_MoveAbsolute_0.Axis = Axis1Obj;
	MC_MoveAbsolute(&MC_MoveAbsolute_0);

	/********************** MC_MOVEADDITIVE *************************/
	MC_MoveAdditive_0.Axis = Axis1Obj;
	MC_MoveAdditive(&MC_MoveAdditive_0);

	/********************** MC_MOVEVELOCITY *************************/
	MC_MoveVelocity_0.Axis = Axis1Obj;
	MC_MoveVelocity(&MC_MoveVelocity_0);

	/************************** MC_STOP *****************************/
	MC_Stop_0.Axis = Axis1Obj;
	MC_Stop(&MC_Stop_0);

	/**************************MC_HALT*******************************/
	MC_Halt_0.Axis = Axis1Obj;
	MC_Halt(&MC_Halt_0);

	/************************** MC_RESET ****************************/
	MC_Reset_0.Axis = Axis1Obj;
	MC_Reset(&MC_Reset_0);
	
	MC_BR_CyclicRead_0.Axis = Axis1Obj;
	MC_BR_CyclicRead(&MC_BR_CyclicRead_0); 

}


