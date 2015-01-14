int  MeasuredLoadLevel=0,DesiredLevel=0,LevelDifference;
void setup()  {
	pinMode(0,  OUTPUT);
	pinMode(1,  OUTPUT);
	pinMode(A0, INPUT);
	pinMode(A5, INPUT);//Used just to get a desired level
}
void loop()  {
	MeasuredLoadLevel=analogRead(A0);
	delayMicroseconds(100);
	DesiredLevel=analogRead(A5);
	delayMicroseconds(100);
	LevelDifference=MeasuredLoadLevel-DesiredLevel;
	//if the load voltage is higher than the measured voltage
	if(LevelDifference>21)
	{//Discharge
		digitalWrite(0,  HIGH);
		digitalWrite(1,  HIGH);
		return;
	}
	//if the measured voltage is higher than the load voltage
	if(LevelDifference<-21)
	{//Charge
		digitalWrite(0,  LOW);
		digitalWrite(1,  LOW);
		return;
	}
	//Otherwise do nothing
	digitalWrite(0,  HIGH);
	digitalWrite(1,  LOW);
}