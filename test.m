[filename,pathname]=uigetfile('*.*','Chon anh dau vao');
filewithpath=strcat(pathname,filename);
img=imread(filewithpath);
imgo=img;
img=rgb2gray(img);
img=imresize(img,[M,N]);
img=double(reshape(img,[1,M*N]));
imgpca=(img-m)*Ppca; %chieu anh goc ban dau len ko gian PCA
distarray = zeros(n,1); %khoi tao mang cho viec so sanh
for i=1:n
    distarray(i)=sum(abs(T(i,:)-imgpca)); %tim khoang cach giua anh train va anh dau vao
end
[result,indx]=min(distarray); %tim duoc anh match tu khoang cach nho nhat
resultimg = imread(sprintf('%d.jpg',indx)); %cho ra duoc anh match tot nhat
%Ve anh ra
subplot(121)
imshow(imgo);
title('Anh dau vao ban dau');
subplot(122);
imshow(resultimg);
title('Anh sau khi nhan dang');
%m, M,N, Ppca, n, T