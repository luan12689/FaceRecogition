
n = input('Nhap so anh can train: '); %n=300 
L = input('Nhap so gia tri Eigen: '); %L=50
M=100; N=90; %Kich thuoc yeu cau cua anh
X=zeros(n,(M*N)); %khoi tao ma tran X ban dau
T=zeros(n,L); %Khoi tao ma tran T trong ko gian PCA
for count=1:n
    I=imread(sprintf('%d.jpg',count)); %doc anh
    I=rgb2gray(I); %chuyen tu anh mau sang anh xam
    I=imresize(I,[M,N]); %chinh lai kich thuoc chuan cho anh
    X(count,:)=reshape(I,[1,M*N]); %chuyen anh ve dang 1D la 1xMN
end
Xb=X; %copy lai anh
m=mean(X); %tinh gia tri trung binh cua cac anh
for i=1:n
    X(i,:)=X(i,:)-m; %tinh phuong sai tu anh 1D
end
Q = (X'*X)/(n-1); %tim ma tran covariance
[Evecm,Evalm] = eig(Q); %tinh gia tri eigen va vector eigen cua ma tran cov Q
Eval = diag(Evalm); %dua cac gia tri eigen vo duong cheo ma tran
[Evalsorted,Index]=sort(Eval,'descend'); %lam giam so cot eigen theo chieu giam dan
Evecsorted=Evecm(:,Index);
Ppca = Evecsorted(:,1:L); %ma tran pca sau khi giam so chieu
for i=1:n
    T(i,:) = (Xb(i,:)-m)*Ppca; %chieu moi anh len PCA space
end