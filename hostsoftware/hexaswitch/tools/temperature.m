# This script for GNU Octave plots temperature logs. 
#

# load the files containing time and temperature values
# first column is timestamp, second column is temperature value
temp1=load("-ascii", 'fe80::2ce:4200:70:428f%usb0');
temp2=load("-ascii", 'fe80::50:c4ff:fe04:8010%usb0'); % kappoth
temp3=load("-ascii", 'fe80::50:c4ff:ff04:8005%usb0'); % hat offset +1°C
temp4=load("-ascii", 'fe80::50:c4ff:fe04:8009%usb0');

%temp1_time=[];

clf;
plot(temp1(:,1),temp1(:,2), "-b;Sensor 1;");
hold on;
# Sensor 2 is way off, so don't plot it.
#plot(temp2(:,1),temp2(:,2)-15.5, "-k;Sensor 2;");
# Sensor 3 has a offset of 1K, so correct this when plotting.
plot(temp3(:,1),temp3(:,2)-1, "-g;Sensor 3;");
plot(temp4(:,1),temp4(:,2), "-r;Sensor 4;");
hold off;

# Add descriptions 
grid;
xlabel('time');
ylabel('\vartheta [°C]');
title('Temperature over time');
time_labels=get(gca(), "xtick");
time_reallabels = [];
for i=1:size(time_labels, 2),
  time_reallabels = [ time_reallabels; strftime('%d.%m %H:%M', localtime(time_labels(1,i))) ];
end;

set(gca(), "xticklabel", time_reallabels);

# uncomment to limit the temperature-axis 
# ylim([23.5 26]);

# save plot as a PDF file
print('temperature.png','-dpng')
