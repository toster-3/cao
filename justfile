default:
	@just -l

#run program
run +ARGS="":
	make
	-./build/cao {{ARGS}}

runo +ARGS="":
	make OFLAGS="-O2"
	-./build/cao {{ARGS}}

#create a header file
addh NAME:
	printf "#ifndef NAME_H_\n#define NAME_H_\n\n#endif\n" > src/{{snakecase(NAME)}}.h
	sed -i s/NAME/{{shoutysnakecase(NAME)}}/g src/{{snakecase(NAME)}}.h

