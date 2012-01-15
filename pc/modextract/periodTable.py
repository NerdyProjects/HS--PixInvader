import sys

fmod_freq = 20000000/12/256
mod_freq = 7093789.2/2
mod_periods = 1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,906,856,808,762,720,678,640,604,570,538,508,480,453,428,404,381,360,339,320,302,285,269,254,240,226,214,202,190,180,170,160,151,143,135,127,120,113,107,101,95,90,85,80,75,71,67,63,60,56

def translate(x):
  delay_mod = x/mod_freq
  fmod_period = 256/(delay_mod*fmod_freq)
  return fmod_period

for p in mod_periods:
  sys.stdout.write(str(int(translate(p) + 0.5)) + ', ')

