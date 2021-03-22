%Code start
clc; 
[filename, folder] = uigetfile ({'*.jpg';'*.bmp';'*.png'}, 'File Selector'); 
fullFileName = fullfile(folder, filename); 
img = imread(fullFileName); 
figure(1);
imshow(img); 
FaceDetect = vision.CascadeObjectDetector; 
FaceDetect.MergeThreshold = 7 ;
BB = step(FaceDetect, img); 
figure(2);
imshow(img); 
for i = 1 : size(BB,1)     
  rectangle('Position', BB(i,:), 'LineWidth', 3, 'LineStyle', '-', 'EdgeColor', 'r'); 
end 
for i = 1 : size(BB, 1) 
  J = imcrop(img, BB(i, :)); 
  figure(3);
  subplot(6, 6, i);
  imshow(J); 
  resize
end
%Code End