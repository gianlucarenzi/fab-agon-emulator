   10 REM Hat Graph
   20 :
   30 CLS
   40 REM VDU 19,1,1,0,0,0
   50 DIM RR(320)
   60 FOR I=0 TO 320:RR(I)=193:NEXT I
   70 XP=144:XR=4.71238905:XF=XR/XP
   80 FOR ZI=64 TO -64 STEP -2
   90   ZT=ZI*2.25:ZS=ZT*ZT
  100   XL=INT(SQR(20736-ZS)+0.5)
  110   FOR XI=0-XL TO XL STEP.5
  120     XT=SQR(XI*XI+ZS)*XF
  130     YY=(SIN(XT)+SIN(XT*3)*0.4)*56
  140     X1=XI+ZI+160:Y1=90-YY+ZI
  150     IF X1<0 THEN GOTO 190
  160     IF RR(X1)<=Y1 THEN 190
  170     RR(X1)=Y1
  180     PLOT 69,X1*4,850-Y1*4+32
  190   NEXT XI:NEXT
