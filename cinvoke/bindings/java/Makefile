TARGET = libcinvoke_java.jnilib

all: $(TARGET)
	javac org/cinvoke/*.java

SRC = org_cinvoke_Natives.cpp
OBJ = $(SRC:.cpp=.o)

$(TARGET): $(OBJ)
	g++ -dynamiclib -o $(TARGET) $(OBJ) -L../../lib -lcinvoke

$(OBJ): $(SRC) org_cinvoke_Natives.h
	g++ -g -Wall -DCINVOKE_BUILD -DARCH_GCC_X64_OSX -dynamic -I../../lib -I/System/Library/Frameworks/JavaVM.framework/Headers -c $(SRC) -o $(OBJ)

header:
	javah org.cinvoke.Natives

clean:
	rm -f org/cinvoke/*.class *.class *.o *.jnilib *.so *.dylib

test:
	javac Test.java
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:. java Test

doc:
	rm -rf javadoc
	javadoc -d javadoc -windowtitle "C/Invoke Java Binding" org/cinvoke/*.java

jar: all
	jar cvf cinvoke_java.jar org/cinvoke/*.class

