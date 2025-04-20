getting this test running requires us to run Graphical Programs Over SSH, which can be done with -Y command when using ssh,so order is to 
1: follow the instructions at the end of this page to install Xming
https://www.cs.utexas.edu/~scottm/cs314/handouts/Sam_Laberge_Lab_Machine_Guide.pdf

2: open a powershell as admin and do 
mkdir \dev
echo x > \dev\tty

3: each time you want to test make sure Xming is running and then in powershell do
$env:DISPLAY="localhost:0"
echo x > \dev\tty
ssh -Y username@hostname

4: once in you need to set path with
PATH=~gheith/public/cs439/bin:$PATH

5: now you can test that graphics work by doing
xeyes
