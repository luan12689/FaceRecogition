%% Face Detection
[file,path]=uigetfile('*.*','Select an image');
img=strcat(path,file);
pic=imread(img);
graypic=rgb2gray(pic);

detectorFace=vision.CascadeObjectDetector();
boundingbox=step(detectorFace,graypic);
detpic=insertObjectAnnotation(pic,'Rectangle',boundingbox,'Face');
subplot(121)
imshow(detpic);
title('Anh dau vao ban dau');
%% Face Recognition

imgGray=rgb2gray(pic);
img2=imresize(imgGray,[M,N]);
img2=double(reshape(img2,[1,M*N]));
imgpca=(img2-m)*Ppca; %chieu anh goc ban dau len ko gian PCA
distarray = zeros(n,1); %khoi tao mang cho viec so sanh
for i=1:n
    distarray(i)=sum(abs(T(i,:)-imgpca)); %tim khoang cach giua anh train va anh dau vao
end
[result,indx]=min(distarray); %tim duoc anh match tu khoang cach nho nhat
resultimg = imread(sprintf('%d.jpg',indx)); %cho ra duoc anh match tot nhat

%% Ve anh ra
if indx <=8
    detectorFace=vision.CascadeObjectDetector();
    boundingbox=step(detectorFace,imgGray);
    detpic2=insertObjectAnnotation(resultimg,'Rectangle',boundingbox,'Le Tien Dat');
    subplot(122);
    imshow(detpic2);
    title('Anh sau khi nhan dang');
elseif indx==9
    detectorFace=vision.CascadeObjectDetector();
    boundingbox=step(detectorFace,imgGray);
    detpic2=insertObjectAnnotation(resultimg,'Rectangle',boundingbox,'Duong Van Tho');
    subplot(122);
    imshow(detpic2);
    title('Anh sau khi nhan dang');

else
    detectorFace=vision.CascadeObjectDetector();
    boundingbox=step(detectorFace,imgGray);
    detpic2=insertObjectAnnotation(resultimg,'Rectangle',boundingbox,'Le Thi Sau');
    subplot(122);
    imshow(detpic2);
    title('Anh sau khi nhan dang');
end
    
    
    