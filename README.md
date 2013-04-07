

<A name="toc1-4" title="rNotify" />
rNotify
=======

Written by Ashwin Raghav <ashwinraghav@gmail.com>.

The Posix standard for file system notifications- inotify, just won't do when you need to monitor file mutations in Distributed/Remote File Systems (RFS) like Gluster for two reasons - 1) It wont scale 2) It wont work. RNotify is a scalable solution with a clear separation of concerns that will help application developers and infrastructure engineers build subscription tools to remote files that are globally namespaced by the RFS. [Here](https://github.com/ashwinraghav/rnotify-c/blob/master/RNotify_Proposal.pdf) is what is in development.

<A name="toc2-34" title="License" />
License
-------
Copy the code if you want to. Credit me if you like to.









