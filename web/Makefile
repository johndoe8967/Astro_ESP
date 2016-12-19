
gz=gzip
dest=build
js=sidClock.js.gz jquery-2.1.4.min.js.gz
src=index.html
transfer=$(src)

%.js.gz: %.js
	cp $^ $(dest)
	$(gz) $(dest)/$^

web: $(js)
	cp $(src) $(dest)
	
ftp: 
	$(foreach x,$(transfer),./ftphelper $(dest)/$(x);)


all: web
clean:
	rm $(dest)/*