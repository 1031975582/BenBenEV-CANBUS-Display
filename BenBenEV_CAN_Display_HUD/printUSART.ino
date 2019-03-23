void printUSART()
{
  char str[7];
  while (Serial.read() >= 0);
  //----------------------------------判断充电或运行-------------------------
  byte j = 0, h = 0, l = 0;
  if (spg == 2 || spg == 3)
  {
    if (debug == 1) km0 = 129;
    if (km0 >= 128 && Current <= -10)
    {
      charging = 1;
      spg = 3;
    }
    else
    {
      charging = 0;
      spg = 2;
    }

  }
  if (spgnow != spg)
  {
    SPG_TPN(spg, 2);
#if !debug
    while (!Serial.find("OK"));
    dispatch();
#else
    delay(300);
    dispatch();
#endif
    spgnow = spg;
    setZero();
    changeFilt();
    lastruntime = millis();
    runtime = lastruntime;
  }



  //-----------------------------------------------------------------------//
  //-------------------------------↓计算↓----------------------------------//
  //-----------------------------------------------------------------------//
  power = (Voltage / 10.0) * ( Current / 10.0) / 100.0; //10倍,kW
  if (abs(Current) < 5) //算内阻
    Voltagei = Voltage;
  if (abs(Current) >= 500 && Voltagei != 0)
  {
    Ri = 1000 * ((float)(Voltagei - Voltage) / Current);
    Voltagei = 0;
    if ((flag & 0x08) == 0)
      flag += 0x08;
  }
  if (charging != 1)
  {
    //energy += power * (0.01 * count185) / 36.0; //单位为kW·h * 1000, energy为int时使用
    energy += power / 10.0 * (0.01 * count185) / 3.6; //单位为W·h, energy为float时使用
    count185 = 0;
    //-------------------------------------------------------------计算使用的SOC
    if (SOCt_BMS != 0)
      used_SOC_BMS += SOCt_BMS - SOC_BMS;
    if (SOCt != 0)
      used_SOC += SOCt - SOC;
  }
  //-------------------------------------------------------------计算平均续航
  if (used_SOC != 0)
  {
    meter_per_SOC = ((ODO - ODObeginForKmallCalc) / (used_SOC / 10.0)) * 100.0;
    kmall = meter_per_SOC / 10.0;
  }
  else
  {
    kmall = KM;
  }
  if (used_SOC_BMS != 0)
  {
    meter_per_SOC_BMS = ((ODO - ODObeginForKmallCalc) / (used_SOC_BMS / 10.0)) * 100.0;
    BMSkmall = meter_per_SOC_BMS / 10.0;
  }
  else
  {
    BMSkmall = KM;
  }
  kmremaining = kmall * (SOC / 1000.0);
  BMSkmremaining = BMSkmall * (SOC_BMS / 1000.0);



  //电压条算法：(BOX的y2-1)-((电压-最小电压)/(最大电压-最小电压))*(条高度-2)
  TractionForce = MotorTorque * 2.683732; //牵引力 = 转矩 / 10 * 齿轮比 / 车轮半径
  //170Nm时，轮上转矩1348.17，牵引力4562.34，轮径591mm
  Voltagebox = 312 - ((Voltage - 2800) / 900.0) * 201;
  powerbox = 312 - (abs(power) / 700.0) * 201;
  TractionForceBox = 312 - (abs(TractionForce) / 4600.0) * 201;
  if (motorspd > -3000)
    spd = abs(motorspd) / 7.3170;//←换胎的话修改这个数值
  ODO = (odo[0] * 65536 + (unsigned long)odo[1] * 256 + odo[2]);
  if (charging != 1)
  {
    if (spd >= 50 &&  TractionForce > 0)
      Efficiency = (TractionForce * (spd / 3.6) / power) * 10.0;
    else
      Efficiency = 0;
  }
  else
  {
    ChgPin = ChgVin * ChgIin/10.0; ///瓦特
    Efficiency = ((Voltage/10.0)*abs(Current/10.0)) / ChgPin * 1000.0;
    if (Efficiency>=999)  Efficiency=999;
  }

  if (ODO != ODObegin)  //算平均电耗
  {
    consumeavg = (energy / 1000.0) / ((ODO - ODObegin) / 10.0) * 10000.0; //10倍
  }
  if (SOC == 0)
    kmremaining = 0;
  if (SOC_BMS == 0)
    BMSkmremaining = 0;
  if (spd > 50 && Current > 0)
    consume = power * (1000.0 / spd);
  else
    consume = -1;

  //-----------------------------------------------------------------------//
  //-----------------------------↑计算↑----------------------------------//
  //-----------------------------------------------------------------------//


  //---电流----//
  if (spg == 2 || spg == 3)
  {
    color = 15;
    if (abs(Current) >= 1000)
      dtostrf(abs(Current) / 10.0, 3, 0, str);
    else
      dtostrf(abs(Current) / 10.0, 2, 1, str);
    if (Current >= 1000) color = 1;
    if (Current >= 0 && Current < 1000) color = 15;
    if (Current < 0) color = 2;
    LABL(48, 29, 65, 114, str, color, 2);
    Current = 0; //显示完电流就清零，debug用
    ex++;

    if (power != powert) {
      dtostrf(abs(power) / 10.0, 2, 1, str);
      LABL(48, 29, 114, 114, str, color, 2);

      if (power < 0 && powert > 0) {
        color = 2;
        LABL(24, 0, 75, 27, "-", color, 1);
        LABL(24, 0, 123, 27, "-", color, 1);
        ex++;
      }
      if (power >= 0 && powert < 0) {
        color = 15;
        LABL(24, 0, 75, 27, "+", color, 1);
        LABL(24, 0, 123, 27, "+", color, 1);
        ex++;
      }
      ex += 4; powert = power;                          //ex max = 7
    }

    if ((flag & 0x80) == 0x80)
    {
      dtostrf(HVACstat, 2, 0, str);
      LABL ( 32, 285, 48, 328, str, 15, 1);
      flag -= 0x80;
      ex += 1;
    }



    if ((ODO != ODOt) || (SOC != SOCt) || ((flag & 0x20) == 0x20))
    {
      if (SOC == 1000)
        dtostrf(SOC / 10.0, 3, 0, str);
      else
        dtostrf(SOC / 10.0, 2, 1, str);
      if (SOC >= 500) color = 2;
      else if (SOC >= 250) color = 4;
      else color = 1;
      LABL(32, 19, 207, 74 , str, color, 2);
      ex++;
      dtostrf(kmremaining, 3, 0, str);//剩余续航
      LABL(32, 174, 207, 225, str, 15, 1);
      dtostrf(kmall, 3, 0, str);//总续航
      LABL(32, 91, 207, 146, str, 15, 1);
      ex += 2;
      SOCt = SOC;
    }
    if ((SOC_BMS != SOCt_BMS) || ((flag & 0x20) == 0x20))
    {

      if (SOC_BMS == 1000)
        dtostrf(SOC_BMS / 10.0, 3, 0, str);
      else
        dtostrf(SOC_BMS / 10.0, 2, 1, str);
      if (SOC_BMS >= 500) color = 2;
      else if (SOC_BMS >= 250) color = 4;
      else color = 1;
      LABL(32, 19, 239, 74 , str, color, 2);
      ex++;
      if ((SOC_BMS != SOCt_BMS) || (ODO != ODOt) || ((flag & 0x20) == 0x20))
      {
        dtostrf(BMSkmremaining, 3, 0, str);//剩余续航
        LABL(32, 174, 239, 225, str, 15, 1);
        dtostrf(BMSkmall, 3, 0, str);//总续航
        LABL(32, 91, 239, 146, str, 15, 1);
        ex += 2;
      }
      if ((flag & 0x20) == 0x20)
        flag -= 0x20;
      SOCt_BMS = SOC_BMS;
    }


    if ((flag & 0x20) == 0x20)
      flag -= 0x20;
    execute();

    if (charging != 1)
    {

    }
    if (Voltage != Voltaget) {
      dtostrf(Voltage / 10.0, 3, 1, str);
      LABL(48, 141, 64, 244, str, 15, 2);
      dtostrf(MaxVolt, 4, 0, str);
      LABL(24, 11, 271, 71, str, 15, 2);
      dtostrf(MinVolt, 4, 0, str);
      LABL(24, 11, 295, 71, str, 15, 2);
      dtostrf(MaxVoltNum, 2, 0, str);
      LABL(24, 122, 271, 147, str, 15, 2);
      dtostrf(MinVoltNum, 2, 0, str);
      LABL(24, 122, 295, 147, str, 15, 2);
      if (Voltaget < Voltage)
      {
        BOXF(405, Voltagebox, 438, 312, 4);
      }
      else
      {
        BOXF(405, 111, 438, Voltagebox - 1, 0);
      }
      ex += 7; Voltaget = Voltage;              //ex max = 20
    }
    execute();

    if (fBatTemp == 1)
    {
      dtostrf(MaxBatProbTemp, 2, 0, str);
      LABL(24, 176, 272, 213, str, 5, 1);
      dtostrf(MinBatProbTemp, 2, 0, str);
      LABL(24, 176, 296, 213, str, 5, 1);
      ex += 2;
      fBatTemp = 0;
    }
    if (fBatNum == 1)
    {
      dtostrf(MaxBatProbTempNum, 2, 0, str);
      LABL(24, 267, 271, 292, str, 5, 1);
      dtostrf(MinBatProbTempNum, 2, 0, str);
      LABL(24, 267, 295, 292, str, 5, 1);
      ex += 2;
      fBatNum = 0;
    }
    execute();
  }

  if (spg == 2) //-----------------------------------------------仅行驶时显示
  {
    if ((flag & 0x08) == 0x08)
    {
      itoa(Ri, str, 10);  //内阻
      LABL(32, 177, 174, 226, str, 15, 2);
      ex += 1;
      flag -= 0x08;
    }
    if ((flag & 0x10) == 0x10)
    {
      if (TractionForce >= 0)
        dtostrf(TractionForce, 4, 0, str);
      else
        dtostrf(TractionForce, 5, 0, str);
      LABL(32, 19, 174, 100, str, 15, 2);
      //LABL(24, 47, 191, 107, str, 15, 2);

      if (TractionForce >= 3000) color = 1;
      else if (TractionForce >= 0) color = 3;
      else color = 2;
      BOXF(443, TractionForceBox, 476, 312, color);
      BOXF(443, 111, 476, TractionForceBox - 1, 0);
      ex += 4;
      flag -= 0x10;
      //dispatch();
    }
    execute();
    if ((flag & 0x40) == 0x40)
    {
      dtostrf(dcdcCurrent / 10.0, 2, 1, str);
      LABL(24, 269, 165, 320, str, 15, 1);
      dtostrf(((dcdcCurrent / 10.0) * 14.5), 3, 0, str);
      LABL(24, 334, 165, 378, str, 15, 1);
      flag -= 0x40;
      ex += 2;
    }
    if (consume != consumet) {
      if (consume == -1) strcpy(str, "--.-");
      else if (consume >= 1000) strcpy(str, "99.9");
      else dtostrf(consume / 10.0, 2, 1, str);
      LABL(48, 141, 114, 224, str, 15, 2);
      ex++;
      consumet = consume;
    }
    if (ODO != ODOt)  //上电后平均能耗
    {
      dtostrf(consumeavg / 100.0, 2, 2, str);
      LABL(32, 293, 131, 370,  str, 15, 1);
      Serial.print("DS32(350,0,'"); dispatch();
      Serial.print(ODO / 10); Serial.print('.'); dispatch();
      Serial.print(ODO % 10); Serial.print("',15,0);"); dispatch();
      ODOt = ODO;
      ex += 2;                                 //ex max = 19;
    }
    dtostrf(energy / 1000.0, 2, 2, str);
    LABL(32, 293, 98, 370, str, 15, 1);

    ex ++;

    dtostrf(spd / 10.0, 3, 1, str); //速度
    if (spd >= 1000) color = 1;
    else color = 15;
    if (fspd == 1)
    {
      LABL(48, 376, 42, 479, str, color, 2);
      fspd = 0;
      ex++;                                   //ex max = 20
    }
    execute();
    dtostrf(Efficiency / 100.0, 2, 1, str);
    //    if (Efficiency  < 1000)
    //    {
    //      str[4] = '%';
    //      str[5] = '\0';
    //    }
    //    else
    //    {
    //      str[5] = '%';
    //      str[6] = '\0';
    //    }
    LABL(32, 219, 5, 277, str, 15, 2);
    ex++;

    for (j = 3; j <= 7; j++)
    {
      if (Temp[j] != Tempt[j])  //j=0-7:3=逆变器，4=蒸发器，5=制热液，6=冷却液，7=回风口
      {
        dtostrf(Temp[j], 2, 0, str);
        LABL(24, 345, 198 + (j - 3) * 24, 382, str, 5, 1);
        ex++;
        Tempt[j] = Temp[j];
        ex++;
        execute();
      }

    }



    if (ODO != ODObegin && spd == 0) //写EEPROM
    {
      unsigned long ODOtemp;
      EEPROM.get(8, ODOtemp);
      if (ODOtemp != ODO)
      {
        EEPROM.put(8, ODO);
        EEPROM.put(4, energy);
        eepromWrote++;
        unsigned int SOCtemp;
        EEPROM.get(28, SOCtemp);
        if (SOCtemp != used_SOC)
        {
          EEPROM.put(28, used_SOC);
          eepromWrote++;
        }
        EEPROM.get(30, SOCtemp);
        if (SOCtemp != used_SOC_BMS)
        {
          EEPROM.put(30, used_SOC_BMS);
          eepromWrote++;
        }
        Serial.print(F("DS16(320,185,'")); Serial.print(eepromWrote); Serial.print("',15,0);");
      }
    }
    //    Serial.print();
  }

  if (spg == 3)//------------------------------------------------仅充电时显示
  {
    if ((hour != hourt) || (minute != minutet)) {
      Serial.print(F("LABL(48,194,114,300,'"));
      Serial.print(hour);
      Serial.print(':');
      if (minute < 10) Serial.print('0');
      Serial.print(minute);
      Serial.print("',15,2);");
      hourt = hour;
      minutet = minute;
      ex += 2;
    }
    //市电电压
    dtostrf(ChgVin, 3, 0, str);
    LABL(24, 345, 40, 390, str, 15, 2);
    //市电电流
    dtostrf(ChgIin / 10.0, 2, 1, str);
    LABL(24, 410, 40, 460, str, 15, 2);
    //市电功率
    dtostrf(ChgPin, 4, 0, str);
    LABL(24, 335, 65, 390, str, 15, 2);
    //充电效率
    dtostrf(Efficiency / 10.0, 2, 1, str);
    LABL(32, 219, 5, 277, str, 15, 2);
    execute();
    //电池温度
    Serial.print(F("DS16(26,179,'")); //字符串保存到PROGMEM
    dispatch();
    //Serial.print("T:  ");
    for (j = 0; j <= 11; j++)
    {
      if ((BTemp[j] > -10) && (BTemp[j] < 10)) comma(' ');
      if (BTemp[j] >= 0) comma(' ');
      Serial.print(BTemp[j], DEC);
      if (j != 11)      comma(' ');
    }

    Serial.print(F("',5,0);"));
    ex += 1;
    execute();
    h = 0;
    for (j = 0; j <= 7; j++)
    {
      if (j == 1 || j == 2 || j == 3) continue;
      if ( Temp[j] != Tempt[j])
      {
        dtostrf(Temp[j], 2, 0, str);
        LABL(24, 345, 198 + h * 24, 382, str, 5, 1);
        ex++;
        Tempt[j] = Temp[j];
      }
      h++;
    }
  }

#if 1
  if (spg == 5) //------------------------------------BMS页面----------占用634字节
  {
    //CELS(m,h,l,'显示内容',c,bc,ali);用m点阵字体在h行l列的单元格中,用c号颜色字,bc号颜色背景,显示相应内容
    //,ali：显示方案 0-居左  1-居中  2-居右
    byte k;
    for (k = 0; k <= 21; k++)
    {
      for (i = 0; i <= 7; i = i + 2)
      {
        BVoltage[j] = BVoltagebuf[k][i] * 256 + BVoltagebuf[k][i + 1];
        j++;
      }
    }
    j = 0;
    for (h = 0; h <= 8; h++)
    {
      for (l = 0; l <= 9; l++)
      {
        if (j == 86) break;
        CELS(l, h, BVoltage[j], 0);
        //        Serial.print("CELS(24,"); Serial.print(l); comma(','); Serial.print(h); Serial.print(",'");
        //        Serial.print(BVoltage[j]); Serial.print("',15,0,1);");
        j++;
        if (j == 25 || j == 50 || j == 75)
        {
          Serial.println();
#if !debug
          while (!Serial.find("OK"));
#else
          delay(75);
#endif
          delay(10);
        }
      }
    }
    Serial.println(); delay(75);
    //    byte maxpos = 0, minpos = 0;
    //    for (j = 0; j <= 85; j++)
    //    {
    //      if (BVoltage[maxpos] < BVoltage[j]) maxpos = j;
    //      if (BVoltage[minpos] > BVoltage[j]) minpos = j;
    //    }
    Serial.print(F("DS24(0,295,'T:  "));
    //    Serial.print("T:  ");
    j = 0;
    for (j = 0; j <= 11; j++)
    {
      Serial.print(BTemp[j], DEC);
      comma(' ');
    }
    Serial.print("     ',15,0);");

    CELS(MaxVoltNum % 10, MaxVoltNum / 10, MaxVolt, 1);
    CELS(MinVoltNum % 10, MinVoltNum / 10, MinVolt, 3);
    //    Serial.print("CELS(24,"); Serial.print(maxpos % 10); Serial.print(','); Serial.print(maxpos / 10); Serial.print(",'");
    //    Serial.print(BVoltage[maxpos]); Serial.print("',15,1,1);");
    //    Serial.print("CELS(24,"); Serial.print(minpos % 10); Serial.print(','); Serial.print(minpos / 10); Serial.print(",'");
    //    Serial.print(BVoltage[minpos]); Serial.print("',15,3,1);");
    Serial.println(); delay(100);
    //ex = 1;
  }
  //------------------------------------↑BMS页面----------
#endif

  Serial.print("SEBL("); Serial.print(bright); Serial.print(",1);");  //SEBL(n,t); t =1 时仅调节背光不存储（可省）
  Serial.println();
  ex = 0;
#if !debug
  while (!Serial.find("OK"));
  dispatch();
#else
  delay(75);
  dispatch();
#endif

}