
# zap 0.1.1.9001

* [9001] [enhance] 2025-07-12 return a data.structure detailing the objects
  which have been serialized.
* [9000] [refactor] 2025-07-12 tidy VECSXP to separate functions for 'raw'
  and 'reference' handling.

# zap 0.1.1  2025-07-11

* [enhance] Bump ZAP_VERSION to 2
    * Added 2 more bytes to header.
    * The first byte is a byte for flags
    * The second bytes is currently unused
    * Only one flag bit currently used:
        * flag to indicate if the writing process made use of list references.
* [enhance] VECSXP (lists and data.frames) can now also be cached by setting
  `list = 'reference'`.  This is not done by default as tracking VECSXPs
  can be costly during both writing and reading.
* [enhance] Environment cache now uses a hashmap
* [bugfix] proper initialization of LGLSXP. bug introduced
  in changeover to new 1-bit handling


# zap 0.1.0   2025-07-02

* Initial release
