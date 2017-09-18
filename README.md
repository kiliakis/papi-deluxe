papi-deluxe

## A C++ library that extends the PAPI library.
* Supports calculation of metrics

## Known bugs
* CORE_BOUND% metric is not properly calculated

## fixed bugs
* Can't add metric or event in the middle of a start - stop counters region
* No function that removes a metric
* Doesn\t work well on PCA and kmeans
* Need a proper destructor