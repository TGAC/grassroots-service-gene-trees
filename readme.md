# Gene Trees service {#gene_trees_service_guide}

The Gene Trees service is for mapping clusters to genes.

## Installation

To build this service, you need the [grassroots core](https://github.com/TGAC/grassroots-core) and 
[grassroots build config](https://github.com/TGAC/grassroots-build-config) installed and configured.

The files to build the Gene Trees service are in the ```build/<platform>``` directory. 

### Linux

If you enter this directory 

```
cd build/linux
```

you can then build the service by typing

```
make all
```

and then 

```
make install
```

to install the service into the Grassroots system where it will be available for use immediately.

## Configuration

To configure the service, you need to specify the MongoDB database and collection names using the ```database```
and ```collection``` keys respectively. As with any service, you can use the *so:image* key to specify the image
file that you want to use as this service's logo. So an example configuration using a database called *foo* and 
a collection called *bar* would be:

~~~json
{
	"so:image": "http://localhost:2000/grassroots/images/search",
	"database": "gstf",
	"collection": "10wheat_genefamilies"
}
~~~
