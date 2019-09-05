function [psdx] = sound_analysis(filename)
 Fs=4000;
 % Read data from text file in float format
 fileID = fopen(filename,'r');
 formatSpec = '%f';
 x = fscanf(fileID,formatSpec);

 

 
 
 % Filter the sound
 %[b,a] = butter(3,([100/Fs,1999/Fs]));
 %noise=filter(b,a,x); 
 %x=x-noise;
 

 
 %Apply Fourier Tansformation
 N = length(x);
 xdft = fft(x);
 xdft = xdft(1:N/2+1);
 psdx = (1/(Fs*N)) * abs(xdft).^2;
 psdx(2:end-1) = 2*psdx(2:end-1);
 freq = 0:Fs/length(x):Fs/2;
 

 plot(freq,10*log10(psdx))
 grid on
 title('Periodogram')
 xlabel('Frequency (Hz)')
 ylabel('Power/Frequency (dB/Hz)')
 
 psdx=10*log10(psdx);

end