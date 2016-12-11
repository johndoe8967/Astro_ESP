var dat, JD, UT, offset, dIM, RA, EOT
var lat, longit, offset, locOffset;
var hours, utHours, minutes, seconds, day, utDay, month, utMonth, year, utYear;

function HoursMinutesSeconds(time) {
	var sign = 1;
	if (time<0) {
		time = -time;
		sign = -1;
	}
	var h = Math.floor(time);
	var min = Math.floor(60.0 * frac(time));
	var secs = Math.round(60.0 * (60.0 * frac(time) - min));

	var str="";
	if (sign == -1) {str = "-"};
	if (h < 10) str = str + "0"+h;
	else  str = str + h;
	if (min >= 10) str = str + ":" + min;
	else str = str + ":0" + min;
	if (secs < 10) str = str + ":0" + secs;
	else str = str + ":" + secs;
	return str;
}	

function daysInMonth(m, y) {
	var n=31
	m=m-1
	if ((m==0) || (m==2) || (m==4) || (m==6) || (m==7) || (m==9) || (m==11))  n=31
	if ((m==3) || (m==5) || (m==8) || (m==10))  n=30;
	if (m==1) {
		n=28;
		if ((y % 4) == 0) n=29
		if ((y % 100) == 0) n=28
		if ((y % 400) == 0) n=29
	}
	dIM=n;
}

function calcLMST() {
	var long = document.getElementById("longitude");
	var dat1 = new Date();
	seconds = dat1.getUTCSeconds();
	minutes = dat1.getUTCMinutes();
	hours   = dat1.getHours();
	utHours = dat1.getUTCHours();
	day		= dat1.getUTCDate();
	month	= dat1.getUTCMonth()+1;
	year	= dat1.getUTCFullYear();
	UT = utHours + minutes/60 + seconds/3600;
	
	longit = long.value;
	
	return LM_Sidereal_Time(JulDay (day, month, year, UT),longit);
}

function calculate() {
	var LMST = document.getElementById("startime");
	var position = document.getElementById("incr1");
	
	var positiontime = position.value / (4*12) / 250 * 20 / 80 * 24;
	if (!(LMST === document.activeElement)) {
		var blah = HoursMinutesSeconds(calcLMST());
		var temp = (calcLMST()-positiontime);
		if (temp >= 24) {
			temp = temp - 24;
		}
		LMST.value = HoursMinutesSeconds(temp);
	}
}

function setLMST() {
	var LMST = document.getElementById("startime");
	var hourfield = document.getElementById("Hour1");
	var position = document.getElementById("incr1");
	var positiontime = position.value / (4*12) / 250 * 20 / 80 * 24;
	
	var d = new Date("1970-01-01T" + LMST.value + "Z");
	var hour= d.getTime()/1000/3600; //- d.getTimezoneOffset()/60;
	var temp = calcLMST() - hour;

	
	if (temp-positiontime >= 12) {
		temp = temp - 24;
	}
	if (temp-positiontime <= -12) {
		temp = temp + 24;
	}
	hourfield.value = HoursMinutesSeconds(temp);
	
	calcPos(hourfield);
}

function JulDay (d, m, y, u){
	if (y<1900) y=y+1900
	if (m<=2) {m=m+12; y=y-1}
	A = Math.floor(y/100);
	JD =  Math.floor(365.25*(y+4716)) + Math.floor(30.6001*(m+1)) + d - 13 -1524.5 + u/24.0;
	return JD
}

function GM_Sidereal_Time (jd) {	
	var t_eph, ut, MJD0, MJD;

	MJD = jd - 2400000.5;		
	MJD0 = Math.floor(MJD);
	ut = (MJD - MJD0)*24.0;		
	t_eph  = (MJD0-51544.5)/36525.0;			
	return  6.697374558 + 1.0027379093*ut + (8640184.812866 + (0.093104 - 0.0000062*t_eph)*t_eph)*t_eph/3600.0;		
}

function LM_Sidereal_Time (jd, longitude) {
	var GMST = GM_Sidereal_Time(jd);			
	var LMST =  24.0*frac((GMST + longitude/15.0)/24.0);
	return LMST;
}

function frac(X) {
	X = X - Math.floor(X);
	if (X<0) X = X + 1.0;
	return X;		
}


