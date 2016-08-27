PROTO_DIR = proto
GOOGLE_PROTOBUF_INCLUDES = protobuf/src
NANOPB_DIR = nanopb/generator
PROTO_INCLUDES = -I$(PROTO_DIR) -I$(GOOGLE_PROTOBUF_INCLUDES) -I$(NANOPB_DIR)/proto

all: nanopb lcdkeypad.pb

nanopb: nanopb_nanopb nanopb_plugin
	cp nanopb/pb.h src
	cp nanopb/pb_common.c src
	cp nanopb/pb_common.h src
	cp nanopb/pb_decode.c src
	cp nanopb/pb_decode.h src
	cp nanopb/pb_encode.h src
	cp nanopb/pb_encode.c src

nanopb_%: $(NANOPB_DIR)/proto/%.proto
	protoc --python_out=$(NANOPB_DIR)/proto $(PROTO_INCLUDES) $<

%.pb: $(PROTO_DIR)/%.proto
	protoc -o$@ $< $(PROTO_INCLUDES)
	python $(NANOPB_DIR)/nanopb_generator.py $@ -L '#include "%s"'
	mv -f $@.h src
	mv -f $@.c src

run:
	platformio run
    
test:
	g++ src/pb_common.c src/pb_decode.c src/pb_encode.c src/lcdkeypad.pb.c test.cpp -o test -Isrc