%SETUP the Serial Port for Communication

% %=====clean up any leftover serial connections==============
clear all
clc
try
    fclose(instrfind) %close any bogus serial connections
end
% 
% %Set COMs and baudrate
COM = 'COM4';
baudrate = 38400; 
stopbits = 1;
databits = 8;

s = serial(COM, 'DataBits', databits, 'BaudRate', baudrate, 'Parity', 'None', 'StopBits', stopbits);
s.FlowControl = 'none';
fopen(s);

%%
%Reading in a Frame
 
numFaces = fscanf(s, '%i'); %number of faces
numPages = fscanf(s, '%i'); % number of pages
perLine = 66;

faces = cell(numFaces,1);
for m=1:numFaces
    pages = zeros(numPages, 528);
    for i=1:numPages
        numLines = fscanf(s, '%i');
        line = zeros(1, perLine * numLines);
        for k=1:numLines
            temp = fscanf(s, '%i');
            line(1,( (k-1)*perLine + 1 ): (k * perLine))= temp';
        end
        pages(i,:)=line;
    end
    faces{m}=pages;
    disp('rx');
end

%%
% show faces
figure;
for i=1:numFaces
    img = zeros(144,176);
    thisFace = faces{i};
    img(1:3:end,:) = thisFace(:,1:176);
    img(2:3:end,:) = thisFace(:,177:352);
    img(3:3:end,:) = thisFace(:,353:528);
    img = img(1:143,:); % get rid of last line
    subplot(1,numFaces,i);
    imshow(img',[0 255])
end

pause(0.1);
%
%% save faces
dat = load('facelib.mat');
facelib = dat.facelib;
for i = 1:numFaces
% for i = 3
    idx = length(facelib)+1;
    facelib{idx} = faces{i};
end
save('facelib','facelib');

%%
%Send all eigenfaces
load('efaces0501.mat');
load('meanFace0501.mat');
faceNums = [2 7 14 16 17 18]; % 0-based (efaceflash num)
numFaces = length(faceNums);

numBytes = 528;

PN=48;
data = cell(PN,1);

facestosend = cell(numFaces,1);
for i = 1:numFaces
    facestosend{i} = F(:,faceNums(i)+1);
end

%%
fprintf(s, '%d\r', numFaces);
display('PRINTED: NUMFACES');
pause(1);
PagesPerFace = PN;

for j=1:numFaces;
    facenum = faceNums(j)
    facevec = facestosend{j};
    for k=1:PN-1
        dataSend=double(facevec(1+(k-1)*528:k*528));
        data{k}=dataSend;
    end
    % last page is special because we are only using 143 lines
    dataSend = double([facevec(1+(PN-1)*528:end); zeros(176,1)]);
    data{PN} = dataSend;
    %When doing a full face
    fprintf(s, '%d\r', facenum);
    display('Printed: FaceNum');
    pause(1);
    fprintf(s, '%d\r', PagesPerFace);
    display('PRINTED: PAGESPERFACE');
    pause(.01);
    for k=1:PagesPerFace
        fprintf(s, '%d\r', numBytes);
        display('PRINTED: numBytes');
        pause(.01);
        dataSend = data{k};
        for i=1:numBytes
             fprintf(s, '%d\r', dataSend(i));
             pause(.0005);
        end
        pause(.01);
        display(k);
    end
    display('COMPLETE');
    pause(2);
end 

%% show eigenface
%%%%% show this eigenface

off = 28;

for n = 1:1
figure;
efacenum = n; %1-based (efaceflash+1)
subplot(1,2,1);
x = load('efaces0501.mat');
trueface = x.F(:,efacenum+off);
imshow(reshape(trueface,176,143),[-128 127]);
title 'real'
subplot(1,2,2);
eface = faces{n}';
eface(eface>127) = eface(eface>127)-256;
imshow(reshape(eface(:),176,144),[-128 127]);
title 'from flash'

efacetrunc = eface(:);
efacetrunc = efacetrunc(1:end-176);

norm(double(trueface) - efacetrunc)
end

%%%%% end show an eigenface
%% outdated

fprintf(s,'%d\r', desFace+1); 

%%
load('efaces0501.mat');
sum(F(:,[6:end,1:5])==-1,1)'