# Gencoder

Encoding & decoding swiss knife.

This project was created because it often happens to me that I would like to modify
some data that's after several layer of encoding. It's not uncommon these days
to see a base64 encoded JSON containing another base64 encoded json, all that
in a x-www-form-urlencoded POST request. This project aims to help you not only
by putting the decoders in one neat package, but also by being able to automatically
reverse the whole process once you edited what you wanted, no matter how far
in the encoding hell you currently are.

## Supported encodings

* UTF-8
* Base64
* x-www-form-urlencoded

## Demo

The app should be self-explanatory enough, but feel free to check the included [DEMO](DEMO)
for a small tour of the features.

## Controls

* *F1* - Open the current document in your $EDITOR
* *F2* - Save the current document to a file
* *F3* - Do all the previous de/encodings in reverse, so you get the format you started with
* *F4* - Open menu of decoders
* *F5* - Open menu of encoders
* *0-9* - Select de/encoder from menu
* *ENTER* - Select a part of a multipart document
* *BACKSPACE or b* - Go back one level (to parent multipart document)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details
