## How It Works

Multimap is implemented as an in-memory hash table which maps each key to a list of values. Keys and values are arbitrary byte arrays. The following schema illustrates the general design:

```
"a" -> (1, 2, 3, 4, 5, 6)
"b" -> (7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17)
"c" -> (18, 19, 20, 21, 22, 23, 24)
```

Putting a key-value pair into the map adds the value to the end of the list associated with the key. If no such list already exists it will be created. Looking up a key then returns a read-only iterator to this list. If the key does not exist or the associated list is empty, the iterator won't yield any value. 

Removing a value from a list means to mark it as deleted, so that iterators will ignore such values. Removed values will remain on disk until an [optimization](#optimization) operation has been run.

Replacing a value is implemented as a remove operation of the value in question followed by a put operation of the new value. In other words the replacement does not happen in-place since the new value is appended to the end of the list. Similar to the physical removal of deleted values, an [optimization](#optimization) operation has to be run to bring the values in certain order back again.

Jump to [Q&A](#qa) if you want to know the rationale behind these design decisions.


## Block Organization

An important aspect of Multimap's design is that all lists are organized in blocks of data. The size of these blocks is fixed and can be chosen when a new instance is created. Typical block sizes in number of bytes are 64, 128, 256, 512, 1024, or even larger, and yes, it has to be a power of two. The schema, therefore, could be updated as follows:

```
"a" -> [1, 2, 3], [4, 5, 6]
"b" -> [7, 8, 9], [10, 11, 12], [13, 14, 15], [16, 17, free]
"c" -> [18, 19, 20], [21, 22, 23], [24, free]
```

At runtime only the last block of each list is held in memory used as a write buffer. Preceding blocks are written to disk as soon as they are filled up (which might be recognized only by the next put operation), replaced by an id for later referencing. A single value can span multiple blocks if it does not fit into the last one. The schema now shows the structure that actually remains in memory.

```
"a" -> b1, [4, 5, 6]
"b" -> b2, b3, b4, [16, 17, free]
"c" -> b5, b6, [24, free]
```

Hence, the total memory consumption of a map depends on

* the number and size of keys
* the number of block ids which is proportional to the number of values
* the block size, because unless the list is empty its last block is always allocated

To estimate the memory footprint the following formula can be used:

```sh
num_blocks = (num_values * avg_value_size) / block_size

mem_keys        = num_keys * avg_key_size
mem_block_ids   = num_blocks * 1.x
mem_last_blocks = num_keys * block_size

mem_total = mem_keys + mem_block_ids + mem_last_blocks
          = num_keys * (avg_key_size + block_size) + (num_blocks * 1.x)
```

Assuming a key set consisting of words from an English dictionary with an average key size of 5 bytes, the factor that has the biggest impact on the total memory footprint is the block size. Therefore, it is important to choose a value that is most suitable for the given use case to prevent running out of memory.

Large block sizes can improve I/O performance since more data is transferred at once. In contrast, large block sizes lead to higher memory consumption at runtime. As a rule of thumb, if a key set is small a larger block size should be chosen and vice versa. Of course, what is small and what is large depends on the given hardware. Try to make estimates and test different settings.

Here are a some examples:

num_keys | avg_key_size | num_blocks | block_size | memory
---------|--------------|------------|------------|-------
x        |x             |x           |x           | x


## Serialization

As mentioned previously, keys and values are arbitrary byte arrays. This has the advantage that Multimap does not need to deal with packing/unpacking of user-defined data types and leaves the door open for compression (currently not applied). For the user this has the advantage that he/she can stick with his/her preferred serialization method.

On the other hand, this approach has the disadvantage that values are seen as [binary large objects](https://en.wikipedia.org/wiki/Binary_large_object) with no type information. In other words a value is treated as one entity and therefore can only be deleted or replaced as a whole. Updates on individual fields of composite types are not possible.

The tutorial pages demonstrate how Multimap can be used together with some popular serialization libraries. For more information please refer to Wikipedia's [comparison of data serialization formats](https://en.wikipedia.org/wiki/Comparison_of_data_serialization_formats).


## Import/Export

Key-value pairs can be imported from or exported to Base64-encoded text files. This feature is useful for data exchange to/from other libraries and frameworks, or for backup purposes. The file format is defined as follows:

* The file is in [CSV format](https://en.wikipedia.org/wiki/Comma-separated_values).
* The delimiter is whitespace or tab.
* Each line starts with a key followed by one or more values.
* Multiple lines may start with the same key.
* Keys and values are Base64-encoded.

Example:

```sh
key1 value1
key2 value2 value3
key1 value4
key3 value5 value6 value7 value8
key2 value9
```

is equivalent to:

```sh
key1 value1 value4
key2 value2 value3 value9
key3 value5 value6 value7 value8
```

is equivalent to:

```sh
key1 value1
key1 value4
key2 value2
key2 value3
key2 value9
key3 value5
key3 value6
key3 value7
key3 value8
```


## Command Line Tool

Multimap also comes with a command line tool that can and should be [installed](#installation) in addition to the actual shared library. The tool allows you to perform some administration operations right in your terminal. Here is the help page:

```plain
$ multimap help
USAGE

  multimap COMMAND PATH_TO_MAP [PATH] [OPTIONS]

COMMANDS

  help           Print this help message and exit.
  stats          Print statistics about an instance.
  import         Import key-value pairs from Base64-encoded text files.
  export         Export key-value pairs to a Base64-encoded text file.
  optimize       Rewrite an instance performing various optimizations.

OPTIONS

  --create       Create a new instance if missing when importing data.
  --bs      NUM  Block size to use for a new instance. Default is 512.
  --nparts  NUM  Number of partitions to use for a new instance. Default is 23.
  --quiet        Don't print out any status messages.

EXAMPLES

  multimap stats    path/to/map
  multimap import   path/to/map path/to/input
  multimap import   path/to/map path/to/input/base64.csv
  multimap import   path/to/map path/to/input/base64.csv --create
  multimap export   path/to/map path/to/output/base64.csv
  multimap optimize path/to/map path/to/output
  multimap optimize path/to/map path/to/output --bs 128
  multimap optimize path/to/map path/to/output --nparts 42
  multimap optimize path/to/map path/to/output --nparts 42 --bs 128


Copyright (C) 2015 Martin Trenkmann
<http://multimap.io>
```

### multimap stats

This command reports statistical information about a map located in a given directory. A typical output reads as follows:

```plain
$ multimap stats path/to/map
#0   block_size        128       ******************************
#0   key_size_avg      8         ******************************
#0   key_size_max      40        ******************************
#0   key_size_min      1         ******************************
#0   list_size_avg     45        *******************
#0   list_size_max     974136    ****
#0   list_size_min     1         ******************************
#0   num_blocks        3170842   ******************
#0   num_keys_total    573308    ******************************
#0   num_keys_valid    573308    ******************************
#0   num_values_total  25836927  *******************
#0   num_values_valid  25836927  *******************

#1   block_size        128       ******************************
#1   key_size_avg      8         ******************************
#1   key_size_max      40        ******************************
#1   key_size_min      1         ******************************
#1   list_size_avg     45        *******************
#1   list_size_max     1010385   ****
#1   list_size_min     1         ******************************
#1   num_blocks        3209400   *******************
#1   num_keys_total    572406    ******************************
#1   num_keys_valid    572406    ******************************
#1   num_values_total  26124736  *******************
#1   num_values_valid  26124736  *******************

[ #2 .. #22 ]

===  block_size        128      
===  key_size_avg      8        
===  key_size_max      40       
===  key_size_min      1        
===  list_size_avg     47       
===  list_size_max     8259736  
===  list_size_min     1        
===  num_blocks        77641183 
===  num_keys_total    13167247 
===  num_keys_valid    13167247 
===  num_values_total  629686802
===  num_values_valid  629686802
===  num_partitions    23
```

Since a map is divided into several partitions there is one info block per partition, followed by a final block that states the total numbers. The asterisks visualize the numbers as relative values with respect to the partition with the maximum value in this category. The output can be filtered with a little help from `grep`. For example, a histogram that shows the distribution of values among the partitions can be generated like this:

```plain
$ multimap stats path/to/map | grep values_total
#0   num_values_total  25836927  *******************
#1   num_values_total  26124736  *******************
#2   num_values_total  25616082  *******************
#3   num_values_total  24602683  ******************
#4   num_values_total  23633504  *****************
#5   num_values_total  25757280  *******************
#6   num_values_total  24665881  ******************
#7   num_values_total  24207863  ******************
#8   num_values_total  36033742  **************************
#9   num_values_total  27674578  ********************
#10  num_values_total  41774200  ******************************
#11  num_values_total  27141930  ********************
#12  num_values_total  27015011  ********************
#13  num_values_total  25249784  *******************
#14  num_values_total  26979204  ********************
#15  num_values_total  25433887  *******************
#16  num_values_total  25533309  *******************
#17  num_values_total  23607578  *****************
#18  num_values_total  23853340  ******************
#19  num_values_total  26713848  ********************
#20  num_values_total  25827632  *******************
#21  num_values_total  24587953  ******************
#22  num_values_total  41815850  ******************************
===  num_values_total  629686802
```

Similarly, to print only the total values you can run:

```plain
$ multimap stats path/to/map | grep =
===  block_size        128      
===  key_size_avg      8        
===  key_size_max      40       
===  key_size_min      1        
===  list_size_avg     47       
===  list_size_max     8259736  
===  list_size_min     1        
===  num_blocks        77641183 
===  num_keys_total    13167247 
===  num_keys_valid    13167247 
===  num_values_total  629686802
===  num_values_valid  629686802
===  num_partitions    23
```

### multimap import

### multimap export

### multimap optimize

An existing Multimap can be optimized using one of the `multimap::Optimize` functions. The optimize operation performs a rewrite of the entire map and therefore, depending on the size of the map, might be a long running task. Optimization has the following effects:

* **Defragmentation**. All blocks which belong to the same list are written sequentially to disk which improves locality and leads to better read performance.
* **Garbage collection**. Values that are marked as deleted won't be copied which reduces the size of the new map and also improves locality.

Optional:

* **Sorting**. All lists can be sorted applying a user-defined `multimap::Callables::Compare` function.
* **Block size**. The new optimized map can be given a different and maybe more suitable block size.


## Q&A

<span class="qa-q" /> Why are all keys held in memory?<br>
<span class="qa-a" />

<span class="qa-q" /> Why are values need to be byte arrays?<br>
<span class="qa-a" />

<span class="qa-q" /> Can I replace values in-place?<br>
<span class="qa-a" /> ...

<span class="qa-q" /> Can I sort a list after a replace operation to restore a certain order of my values?<br>
<span class="qa-a" /> ...

<span class="qa-q" /> Can I use Multimap as a 1:1 key value store?<br>
<span class="qa-a" /> Multimap can be used as a 1:1 key-value store as well, although other libraries may be better suited for this purpose. As always, you should pick the library that best fits your needs, both with respect to features and performance. Finally, when using Multimap as a 1:1 key-value store, you should set the block size as small as possible to waste as little space as possible, because each block will only contain just one value. Also keep in mind that the block size defines the maximum size of a value.