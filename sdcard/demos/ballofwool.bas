10 REM BALLOFWOOL
20 MODE 3
30 S%=500
40 VDU29,640;512;
50 MOVE0,0
60 GCOL0,RND(63)
70 FORA=0 TO 125.7 STEP 0.1
80 DRAW S%*SIN(A),S%*COS(A)*SIN(A*0.95)
90 NEXT
100 GOTO 50

