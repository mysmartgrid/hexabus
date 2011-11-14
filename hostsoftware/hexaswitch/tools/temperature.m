# This script for GNU Octave plots temperature logs. 
#

# load the files containing time and temperature values
# first column is timestamp, second column is temperature value
load temp8005;
load temp8009;
load temp8010;
load temp428f;


plot(temp8005(:,1),temp8005(:,2), "-b;Sensor 1;");
hold on;
plot(temp8009(:,1),temp8009(:,2), "-r;Senor 2;");
plot(temp8010(:,1),temp8010(:,2), "-g;Sensor 3;");
hold off;

# Add descriptions 
grid;
xlabel('t / s');
ylabel('\vartheta / °C');
title('Temperature over time');

# uncomment to limit the temperature-axis 
# ylim([23.5 26]);

# ave plot as a PNG file
print('temperature.png','-dpng')
