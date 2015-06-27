# Virtual Escrow Service

This is just a quick and dirty example of exchanging virtual goods between two untrusted peers. I got the idea while trading Pokemon and wondering how one might write
an online service that can handle such an exchange of local data between two arbitrary endpoints with a minimum of exploitation.

Currently it's only designed to build on Linux.

## Dependencies

 * [CMake 3.0](http://www.cmake.org/)
 * [SQLite 3.8.10.2](https://www.sqlite.org/) (included in repository)
 * [Google Protobuf 2.6.1](https://github.com/google/protobuf)
 * [FLTK 1.3.3](http://www.fltk.org/index.php)

## License

The service is licensed under the [CC0 1.0 universal license](LICENSE.md). Dependencies licensed separately.