data=read.table("benchmark.log", header=TRUE, sep=",");
plot(data$Percentage.served, data$Time.in.ms);
