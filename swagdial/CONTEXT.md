# Context for Claude Code

We need to write a project that runs on the M5 Dial 1.1 device.  Use the existing project in the `cheerdial` folder as an example of something similar.

At a high level, this project should:

* Connect to a configurable wifi SSID (consider using a JSON file for this unlike `cheerdial` which uses `config.h`).  Use `config.json` for this.
* Allow the user to choose from a hierarchical menu of items, for example t-shirt -> logo -> s | m | l | xl.  User should use the dial and press button for this etc.
* Don't assume all items have the same structure, e.g. as well as t-shirt with designs and sizes we might have sticker -> logos -> mimir | grafana | tempo etc, as well as sticker -> grot -> sunglasses | guitar | lgbt.  Basically assume each structure is a branch and leaf with arbitrary things.
* Each item in the hierarchy should support an optional image associated with it.  Store the name of the image in the file, assume that the image files themselves will be available on the device in a folder named "images" on the device flash.  Assume JPEG images, show images and text if an image is defined, show only test if it isn't or if the image file can't be found.  Assume a sensible naming convention for the images that maps to the hierarchy.
* Be configurable from a JSON file that describes the hierarchy and items available - call this `items.json` it should describe the item name then any hierarchy under this should translate to the labels that are used on the metric.
* Let's call the high level metric `gcon_swag` unless there are Prometheus best practices that would suggest `-` rather than `_` etc.  If best practices exist, use them.
* Allow the user the nagivate up and down the menu hierarchy so they can go back a level at any point.  Do this by making the last item in every level that isn't the root a "Back" item.  When navigating levels, a press on an item that has leaves under it takes the user to those leaves.  The only finally selectable items that record metrics are the leaf items.
* Allow the user to select an item once they reach a "leaf node" in the hierarchy e.g. a size l logo t shirt
* When a choice is made, send the choice to a prometheus remote write endpoint (there's code in `cheerdial` for this).  This endpoint should be configurable in the same way that the wifi is.  Assume a similar configuration to `cheerdial` - it requires the same sort of authentication.  Put this information in `config.json`.
* Assume one item is chosen at a time.
* Once an item is chosen, provide feedback that the selection was recorded successfully then return to the top of the hierarchy.  So the value that's sent is 1, with labels identifying where in the hierarchy the user was when they selected a leaf.

Here's some things to think about:

* We'll want to know that the device has successfully connected to the wifi (again see code in `cheerdial` for this).
* We don't want to store counters for how many of each item have been selected on a given device.  Assume this project will run on multiple devices, and that aggregation will be done on the prometheus remote write server (this is actually going to be Grafana Mimir in the cloud).  Whilst it's not a primary aim, let's store the device ID as a label in each metric.  Assume the device ID comes from `config.json`.
* We don't want to hard code anything about items in the hierarchy or metrics.  So, make the metric name configurable in `items.json` too.  Assume that all items in the file use that name.
* When designing metric names and patterns, make sure we can easily use them to build a Grafana dashboard later that shows things like: 
  * How many items have been selected total
  * How many t shirts of any size for example
  * How many t shirts of a particular design but any size
  * How many large t shirts etc
  * Items over time etc
  * And other suitable metrics.
* Don't build a dashboard now, but do think about this when designing what metric is sent.
