#!/usr/bin/python -tt

#parse_simple.py:for simple AEs
import sys
import os
next = None
src = ""
labelVal = 0
whileLab = 0 #I wouldn't want my if/while labels to overlap. Keep them
             #independent from eachother

tempVal = 36 #The step before the beginning of the temp value.

#Ugh, I should've had them mapped from 1-26 and do simple math, oh well.  
variable = {'a' : '80($s1)', 'b' : '88($s1)', 'c' : '96($s1)', 'd' : '104($s1)',
         'e' : '112($s1)', 'f' : '120($s1)', 'g' : '128($s1)', 'h' : '136($s1)',
         'i' : '144($s1)', 'j' : '152($s1)', 'k' : '160($s1)', 'l' : '168($s1)',
         'm' : '176($s1)', 'n' : '184($s1)', 'o' : '192($s1)', 'p' : '200($s1)',
         'q' : '208($s1)', 'r' : '216($s1)', 's' : '224($s1)', 't' : '232($s1)',
         'u' : '240($s1)', 'v' : '248($s1)', 'w' : '256($s1)', 'x' : '264($s1)',
         'y' : '272($s1)', 'z' : '280($s1)'}



mips = os.open("mips.s", os.O_CREAT | os.O_WRONLY | os.O_TRUNC)
mips = os.fdopen(mips, 'w')

def M():
   global src
   string = "\t.globl main\n\nmain:\n\t\tmove\t$s7, $ra\n\t\tla\t\t$s1, M\n\n\t\tli\t\t$v0, 4\n\t\tla\t\t$a0, Name\n\t\tsyscall\n\n\n\n"

   mips.write(string)
   string = "" #clear out the initial string
 
   
   scan()
   S()                  
   while (next != '$'):
      src = ""
      tempVal = 36 # might be needed
      S()

   if next != '$':
      print("There's a missing '$' character!")
      error(1)
   else:
      print("End of Program")

      string = "\n\n\n\n\t\tmove\t$ra, $s7\n\t\tjr\t\t$ra\n\n\n\npow:\n\n\ttrunc.w.d $f4, $f4\n\tcvt.d.w $f4, $f4\n\tl.d\t$f6, 8($s1)\n\n\n\tl.d\t$f8, 0($s1)\n\tc.eq.d\t$f4, $f8\n\tbc1t\tend\n\n\tl.d\t$f8, 0($s1)\n\tc.lt.d\t$f8, $f4\n\tbc1t\tnext\n\tl.d\t$f8, 8($s1)\n\tdiv.d\t$f2, $f8, $f2\n\tneg.d\t$f4, $f4\n\nnext:\tl.d $f8, 0($s1)\n\tc.eq.d\t$f4, $f8\n\tbc1t\tend\n\tmul.d\t$f6, $f6, $f2\n\tl.d\t$f8, 8($s1)\n\tsub.d\t$f4, $f4, $f8\n\tb next\nend:\tjr\t$ra\n\t\t.data\n\t\t.align\t\t3\n\nM:\t\t.double 0.,1.,2.,3.,4.,5.\n\t\t.double 6.,7.,8.,9. #constants\n\t\t.space\t208\n\t\t.space\t1000\nBlank:\t.asciiz \" \"\nNewL:\t\t.asciiz \"\\n\"\nTab:\t\t.asciiz \"\\t\"\nName:\t\t.asciiz \"Executed by Dylan Soderman\\n\""
      mips.write(string)

      mips.close()
      exit(0)


#Difference with this now is that S() doesn't perform the scan.
#This eliminates problems with "next" being unintentionally evaluated
#from an earlier recursion. 
def S():
   global mips
    
   #Take a character, see if it fits with these cases....
   if (next.islower()):
      A() 
   elif(next == '<'): #output
      P()
   elif(next == '>'): #input
      G() 
   elif(next == '['):   #If statement
      I()
   elif(next == ']'):
      print(" ")  #Do nothing
   elif(next == ':'):
      print(" ")
   elif(next == '{'):   #While Statement
      W()
   elif(next == '$'):
      print(" " )
   else:
      print("Error at function S")
      error(2)

   mips.write('\n')



def I():
   scan()
   ifLabel   = 'IfEnd'   + str(labelVal) 
   elseLabel = 'elseEnd' + str(labelVal) 
   incLabel()
   done = 0 #Boolean for preventing outer if's from evaluating inner if's
                                 #else statements

   oneString = E()  #oneString  #Load up the statement
   
   oneString = '\tl.d\t$f2, ' + oneString
   oneString = oneString + '\tl.d\t$f4, 0($s1)\n\tc.eq.d\t$f2, $f4\n\tbc1t\t'
   oneString = oneString + ifLabel + '\n'


   mips.write(oneString)
   sys.stdout.write(oneString)


   if(next == '?'):

      scan()
      S()              
      while (next != '$' and next != ':' and next != ']'):
         src = ""
         S()

      #If no else statement, just write the label to jump to.
      if(next == ']'):
         mips.write(ifLabel + ':\n')
         sys.stdout.write(ifLabel + ':\n')
      #Otherwise, have a unconditional jump to skip the else statements.
      elif(next == ':'):
         mips.write('\tj\t' + elseLabel + '\n')
         sys.stdout.write('\tj\t' + elseLabel + '\n')

         mips.write(ifLabel + ':\n')
         sys.stdout.write(ifLabel + ':\n')
         
         scan()
         S()                  
        
         while (next != '$' and done == 0):
            src = ""
            if(next == ']'):
               mips.write(elseLabel + ':\n')
               sys.stdout.write(elseLabel + ':\n')
               done = 1

            S()
            

   else:
      print("Need '?' character in tiny statement!") 

   #Scan for next character when function ends. Stick with how the
   #other functions are written
   scan()

def W():
   scan()
   whileStart   = 'whileStart'   + str(whileLab) 
   whileEnd     = 'whileEnd'     + str(whileLab)
   incWhile()

   mips.write(whileStart + ':\n')
   sys.stdout.write(whileStart + ':\n')


   oneString = E()  #oneString  #Load up the statement
   
   oneString = '\tl.d\t$f2, ' + oneString
   oneString = oneString + '\tl.d\t$f4, 0($s1)\n\tc.eq.d\t$f2, $f4\n\tbc1t\t'
   oneString = oneString + whileEnd + '\n'


   mips.write(oneString)
   sys.stdout.write(oneString)

   if(next == '?'):

      scan()
      S()               
      while (next != '$' and next != '}'):
         src = ""
         S()

      #Much shorter stuff. 
      if(next == '}'):
         mips.write('\tj\t' + whileStart + '\n')
         sys.stdout.write('\tj\t' + whileStart + '\n')

         mips.write(whileEnd + ':\n')
         sys.stdout.write(whileEnd + ':\n')


      if(next == '$'):
         print("Error! Missing '}' pair!")


   else:
      print("Need '?' character in tiny statement!") 

   scan()


def A():

   #We already know it's a lowercase letter, else how are we in A()?
   oneString = variable[next]
   original = oneString
   

   scan() 
   while(next.islower()): #Accept variables of multiple letters...but don't 
      scan()              #create mips code out of it! 
   if(next == '='): #Variable is defined.

      scan() #This section will be oneString = E() + oneString  

      #Result with all the values stored in should be returned. 
      oneString =  E() 
      oneString = '\tl.d\t$f2, ' + oneString

      oneString = oneString  + '\ts.d\t$f2, ' + original + '\n'


      mips.write(oneString)
      sys.stdout.write(oneString)

      if(next != ';'):
         error(3)

      scan()
   else:
      print("We ain't got no '=' sign!")
      error(3)


def P():
   oneString = ""
   scan()
   if((next is '+' or next is '-' or  next.isalnum()) and next is not 'N' and next is not 'B' and next is not 'T'):   #Need to print value
      oneString =  "\tli\t$v0, 3\n"

      oneString = oneString + '\tl.d\t$f12, ' +  E() 

   elif(next == 'N'):
      oneString = "\tli\t$v0, 4\n"
      oneString = oneString + "\tla\t$a0, NewL\n"
      scan()
   elif(next == 'B'):
      oneString = "\tli\t$v0, 4\n"
      oneString = oneString + "\tla\t$a0, Blank\n"
      scan()
   elif(next == 'T'):
      oneString = "\tli\t$v0, 4\n"
      oneString = oneString + "\tla\t$a0, Tab\n"
      scan()


      #Add Case for 'B"
   if(next != ';'):
      error(3)

   scan()   

   oneString = oneString + '\tsyscall\n\n'
   mips.write(oneString)
   sys.stdout.write(oneString)

def G():
   scan()
   if(next.islower()):


      value =  variable[next] + '\n'

      mips.write('\tli\t$v0, 7\n\tsyscall\n')
      sys.stdout.write('\tli\t$v0, 7\n\tsyscall\n')

      mips.write('\ts.d\t$f0, ' + value)
      sys.stdout.write('\ts.d\t$f0, ' + value)

      scan() #acquire ';' character

      if(next != ';'):
         print("Need a semicolon for your input statement!")
         error(3)


   else:
      print("Need to specify a letter variable in your tiny code for input!")
      error(3)

   #acquire next character
   scan()


def E():
  
   value = ""
   value =  T()
   while next == '+' or next == '-':

      letter = next
      arg1 = value;
      scan()
      arg2 = T()


      if(letter == '+'):
         
         
         string =  '\tl.d\t$f2, ' + arg1
         sys.stdout.write(string)

         mips.write('\tl.d\t$f2, ' + arg1)
         mips.write('\tl.d\t$f4, ' + arg2)

         string =  '\tl.d\t$f4, ' + arg2
         sys.stdout.write(string)
 

         mips.write('\tadd.d\t$f2, $f2, $f4\n')
         sys.stdout.write('\tadd.d\t$f2, $f2, $f4\n')

      elif(letter == '-'): #Subtraction


         string =  '\tl.d\t$f2, ' + arg1
         sys.stdout.write(string)

         string =  '\tl.d\t$f4, ' + arg2
         sys.stdout.write(string)
 

         mips.write('\tl.d\t$f2, ' + arg1)
         mips.write('\tl.d\t$f4, ' + arg2)

         mips.write('\tsub.d\t$f2, $f2, $f4\n')
         sys.stdout.write('\tsub.d\t$f2, $f2, $f4\n')


      value = str(tempVal*8) + '($s1)\n\n'
      mips.write('\ts.d\t$f2, ' + value)

      sys.stdout.write('\ts.d\t$f2, ' + value)

    
    

   return value

def T():


   value = ""
   value =  U()
   while next == '*' or next == '/' or next == '@' or next == '%':

      letter = next
      arg1 = value;
      scan()
      arg2 = U()


      if(letter == '*'):
         
         
         string =  '\tl.d\t$f2, ' + arg1
         sys.stdout.write(string)

         mips.write('\tl.d\t$f2, ' + arg1)
         mips.write('\tl.d\t$f4, ' + arg2)

         string =  '\tl.d\t$f4, ' + arg2
         sys.stdout.write(string)
 
                             #$f2
         mips.write('\tmul.d\t$f6, $f2, $f4\n')
         sys.stdout.write('\tmul.d\t$f6, $f2, $f4\n')

      elif(letter == '/' or letter == '@' or letter == '%'): #Division!


         string =  '\tl.d\t$f2, ' + arg1
         sys.stdout.write(string)

         string =  '\tl.d\t$f4, ' + arg2
         sys.stdout.write(string)
 

         mips.write('\tl.d\t$f2, ' + arg1)
         mips.write('\tl.d\t$f4, ' + arg2)

         mips.write('\tdiv.d\t$f6, $f2, $f4\n')
         sys.stdout.write('\tdiv.d\t$f6, $f2, $f4\n')

         if(letter == '@' or letter == '%'):# Truncating that division!

            sys.stdout.write('\ttrunc.w.d\t$f6, $f6\n')
            sys.stdout.write('\tcvt.d.w\t$f6, $f6\n')
         
            mips.write('\ttrunc.w.d\t$f6, $f6\n')
            mips.write('\tcvt.d.w\t$f6, $f6\n')


            if(letter == '%'):  #Getting the remainder only!

               sys.stdout.write('\tmul.d\t$f6, $f6, $f4\n')
               sys.stdout.write('\tsub.d\t$f6, $f2, $f6\n')

               mips.write('\tmul.d\t$f6, $f6, $f4\n')
               mips.write('\tsub.d\t$f6, $f2, $f6\n')
         


      value = str(tempVal*(8)) + '($s1)\n\n'
      increment()
                        #$f2
      mips.write('\ts.d\t$f6, ' + value)
      sys.stdout.write('\ts.d\t$f6, ' + value)

   return value


def U():
   value = ""
   value = F()
   
   while next == '^':
      #mips.write('\tl.d\t$f2, ' + value)
      #sys.stdout.write('\tl.d\t$f2, ' + value)

      scan()
      exponent = U()

      mips.write('\tl.d\t$f2, ' + value)
      sys.stdout.write('\tl.d\t$f2, ' + value)


      
      mips.write('\tl.d\t$f4, ' + exponent)
      sys.stdout.write('\tl.d\t$f4, ' + exponent)

      mips.write('\tjal\tpow\n')
      sys.stdout.write('\tjal\tpow\n')



      value = str(tempVal*(8)) + '($s1)\n\n'
      increment()
                        #$f2
      mips.write('\ts.d\t$f6, ' + value)
      sys.stdout.write('\ts.d\t$f6, ' + value)






   return value


def F():
   value = ""
   if(next.isalnum()): # alphanum2

      if(next.islower()): #a variable
         value =  variable[next] + '\n'

      elif(next.isnumeric()): #an immediate value
         value =  str(int(next)*8) + '($s1)\n' 
      scan()
   elif next == '(':
      scan()
      increment()
      value = E() 

      if next == ')':
         scan()
      else:
         error(2)

   elif next == '-' or next == '+': #unary value
      while(next == '-' or next == '+'):
         
         scan()
         if(next == '-'):
            value = F()
            mips.write('\tneg.d\t' + value)
            sys.stdout.write('\tneg.d\t' + value)

   else:
      error(3)

   return value                                                                                                                                                                                                                
def error(n):
   sys.stdout.write("ERROR:" + str(n))
   sys.stdout.write(", SOURCE:" +
      src + "\n")
   sys.exit(1)



def getch():
   c = sys.stdin.read(1)
   if len(c) > 0:
      return c
   else:
      return None


def scan():
    global next
    global src
    next = getch()
    if next == None:
         sys.exit(1)
    while next.isspace(): # skip whitesp
         next = getch()
    src += next

#Wouldn't want to declare tempVal in all my functions. 
def increment():
   global tempVal
   tempVal = tempVal + 8
   return tempVal

def incLabel():
   global labelVal
   labelVal = labelVal + 1
   return labelVal

#Because I don't want to pass a boolean value into incLabel
def incWhile():
   global whileLab
   whileLab = whileLab + 1
   return whileLab

while True:
   M()

