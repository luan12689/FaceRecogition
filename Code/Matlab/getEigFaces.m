%% make eigenfaces
desMean = 80; % used to be 65
desStd = 5000;
showTrainFaces = 0;
showEigFaces = 0;
showProj = 0;

% Which people to use for what (make sure no test face is in training set!)
trainidx = [1:17 19:21 23:25 27 30:42 44:45]; % which people to use as trainers
numEigs = 30; % number of eigenfaces to use from the training set
testidx = []; % unknown faces
for j = 1:57
    if (~any(trainidx==j))
        testidx = [testidx j];
    end
end

dat = load('facelib.mat'); % load all pictures (2 per person)
facelib = dat.facelib; % page-by-page representation

trainingfaces = facelib(2*trainidx); % use first picture of each person

if (showTrainFaces)
    figure
end
A = zeros(176*143,length(trainingfaces));
for i = 1:length(trainingfaces);
    thisFace = trainingfaces{i}';
    facevec = thisFace(:);
    % take out last line
    facevec = facevec(1:end-176,:);
    % normalize mean and std of this vector
    thisMean = mean(facevec);
    thisStd  = sqrt(sum((facevec-thisMean).^2));
    facevec = (facevec-thisMean)*desStd/thisStd + desMean;
    % store in matrix
    A(:,i) = facevec;
    if showTrainFaces
        subplot(6,8,i);
        img = reshape(facevec,176,143);
        imshow(img,[0 255]);
    end
end

% mean image vector
meanFace = mean(A,2);

% subtract mean image from each image
T = A-repmat(meanFace,1,size(A,2));
% Find eigenvectors of T*T'. First, find eigenvectors of T'*T.
[Vtil, D] = eig(T'*T);
% Eigenvectors of T*T' are T*V (properly normalized to unit length)
V = T*Vtil;
for i = 1:size(V,2)
    len = sqrt(sum(V(:,i).^2));
    V(:,i) = V(:,i)./len;
end

% Reverse order of eigenfaces and eigenvalues
V = V(:,end:-1:1);
evals = diag(D);
evals = evals(end:-1:1);

% only take the eigenvectors with the highest eigenvalues
F = V(:,1:numEigs);

% shift to uint8
F = F*127/0.03; % empirical factor to preserve most information
F = int8(F);

% show eigenfaces
if showEigFaces
    figure;
    mnV = min(V(:));
    mxV = max(V(:));
    for i = 1:numEigs
        img = F(:,i);
        subplot(6,8,i);
        imshow(reshape(img,176,143),[-128 127]);
    end
end

% Now microcontroller simulation
desStd = int16(desStd);
desMean = int16(desMean);
meanFace = int16(meanFace);

% Register each unknown person's first image
regTemplates = zeros(numEigs,length(testidx),'int32');
if (showProj)
    figure;
end
for i = 1:length(testidx)
    thisFace = int16(facelib{2*testidx(i)-1}');
    facevec = int16(thisFace(:));
    facevec = facevec(1:end-176,:); % take out last line
    % normalize mean and std of this vector
    thisMean = int16(mean(facevec));
    thisStd  = int16(sqrt(sum((double(facevec)-double(thisMean)).^2)));
    stdMult = thisStd/100;
    facevec = (facevec-thisMean)*50/stdMult + desMean;
%     facevec = single(facevec-thisMean)*single(desStd)/single(thisStd) + single(desMean);

    normface = facevec - meanFace;
    for j = 1:numEigs
        regTemplates(j,i) = dot(int16(F(:,j)),normface);
    end
    if (showProj)
        % project onto eigenspace
        projimg = F*(F'*(facevec-meanFace))+meanFace;
        % show both images
        subplot(5,8,2*i-1);
        imshow(reshape(facevec,176,143),[0 255]);
        subplot(5,8,2*i);
        imshow(reshape(projimg,176,143),[0 255]);
    end
end
regTemplates = int16(regTemplates/610);

% now try to login with second picture of each person
loginTemplates = zeros(numEigs,length(testidx),'int32');
for i = 1:length(testidx)
    thisFace = int16(facelib{2*testidx(i)}');
    facevec = int16(thisFace(:));
    facevec = facevec(1:end-176,:); % take out last line
    % normalize mean and std of this vector
    thisMean = int16(mean(facevec));
    thisStd  = int16(sqrt(sum((double(facevec)-double(thisMean)).^2)));
    stdMult = thisStd/100;
    facevec = (facevec-thisMean)*50/stdMult + desMean;
%     facevec = single(facevec-thisMean)*single(desStd)/single(thisStd) + single(desMean);
    
    contsave = [];
    normface = facevec - meanFace;
%     for page = 1:48
        contribution = int32(0);
        for j = 1:numEigs
%             idx = 528*(page-1)+1:528*page;
%             if page == 48
%                 idx = 528*(page-1)+1:25168;
%             end
%             contribution = int32(dot(int16(F(idx,j)),normface(idx))/100);
%             loginTemplates(j,i)=loginTemplates(j,i)+contribution;
%             contsave = [contsave;contribution];
        
        
        loginTemplates(j,i) = dot(int16(F(:,j)),normface)/100;
        end
%     end
end
loginTemplates = int16(loginTemplates/610);


% Simulate login attempt for each person.  Each person's login is a dot
% product with the registered templates.  Each login results in a row
% vector of correlations with the registered templates.
C = zeros(length(testidx)); % correlation matrix
for i = 1:length(testidx)
    for j = 1:length(testidx)
        t1 = single(loginTemplates(:,i));
        t2 = single(regTemplates(:,j));
%         C(i,j) = pi/2 - acos(dot(t1,t2)/(norm(t1)*norm(t2)));
        C(i,j) = dot(t1,t2)/sqrt(sum(t1.^2))/sqrt(sum(t2.^2));
%         C(i,j) = dot(t1,t2);
%         C(i,j) = -sqrt(sum((t1-t2).^2));
    end
end
% show simulated logins
figure;
surf(C)
xlabel 'login #'
ylabel 'registration #'
view(0,90)
caxis([0 1])
colorbar;

% show histograms of stuff
% off = C - diag(diag(C));
% off = off(:);
% on = diag(C);
% bins = linspace(min(C(:)),max(C(:)),100);
% figure;
% subplot(1,2,1);
% hist(off,bins,'b');
% subplot(1,2,2);
% hist(on,bins,'r');

%% show a person
i = 46; % person
figure;
f = facelib{2*i-1}';
f = f(:);
subplot(1,2,1);
imshow(reshape(f,176,144),[0 255]);
f = facelib{2*i}';
f = f(:);
subplot(1,2,2);
imshow(reshape(f,176,144),[0 255]);





%% other stuff



 % second picture of each person
facemat2 = zeros(176*144,length(facelib)/2);
for i = 2:2:length(facelib)
    thisFace = facelib{i}';
    facemat2(:,i/2) = thisFace(:);
end
    img(1:3:end,:) = thisFace(:,1:176);
    img(2:3:end,:) = thisFace(:,177:352);
    img(3:3:end,:) = thisFace(:,353:528);
    img = img'; % get rid of last line
    
%% show eigenfaces
% figure();
for i = 1:1:30
    img = F(:,i);
        figure();
        imshow(reshape(img,176,143),[-128 127]);
%     imshow(reshape(img,176,143),[0 255]);
end
%     img(1:3:end,:) = thisFace(:,1:176);
%     img(2:3:end,:) = thisFace(:,177:352);
%     img(3:3:end,:) = thisFace(:,353:528);
%     img = img'; % get rid of last line
    