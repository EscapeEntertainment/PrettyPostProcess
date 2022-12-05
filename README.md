![Pretty Post Process](PPPBanner.jpg)
===

Pretty Post Process is a UE5 plugin that provides custom bloom and lens flare solution, and making your Unreal Engine game stand out more from the rest.

This plugin is an implementation of [Froyok's set of articles](https://www.froyok.fr/blog/2021-12-ue4-custom-bloom/page.html). As this plugin requires
[engine modification](https://github.com/EscapeEntertainmentTeam/UnrealEngine/commit/4d349e035a6387a3342ae74d7cf9b0f6fc053e62), this will only work when
you build the engine from [the source code](https://github.com/EpicGames/UnrealEngine). If both links appear as 404, follow
[this instruction](https://www.unrealengine.com/en-US/ue-on-github) on binding your Epic Games account to your GitHub account and getting access to UE source code.

# Installation

To install this plugin, you can either:

* Download as ZIP, and unzip it in your project's `Plugins/PrettyPostProcess` directory (create one if it doesn't exist)
* Submodule this repository into your project's `Plugins` directory.

You can also install the plugin into the engine's Plugins directory if you want to applied it to other project of yours that use the same build.

## Engine modification

In order to make the plugin actually work, you have to modify the engine's source code to allow the plugin to 
[override the vanilla bloom and lens flare pass](https://github.com/EscapeEntertainmentTeam/UnrealEngine/commit/4d349e035a6387a3342ae74d7cf9b0f6fc053e62).
Once you apply the changes and compile the engine, you can enable the plugin and see it working.

# Usage

## Data Asset

The plugin is mainly controlled with a Data Asset resides in the plugin's Contents folder, named `DA_PostProcess_Default`. This Data Asset contains parameters
that adjust how the lens flares look.

> **WARNING:** The Data Asset that came with this repo is saved with Unreal Engine 5.1. This will not appear in prior engine versions, and you will have to make
> one yourself with the exact same name.

## Console commands

The plugin offers console commands to control parts of the bloom and lens flares:
- `r.PrettyPostProcess.BloomPassAmount` : Maximum number of passes to render bloom.
- `r.PrettyPostProcess.BloomResLimit` : Minimum downscaling size for the Bloom. This will affect how large the bloom will be..
- `r.PrettyPostProcess.BloomRadius` : Size/Scale of the Bloom.
- `r.PrettyPostProcess.RenderFlare` : Whether to render the lens flare/ghosts.
- `r.PrettyPostProcess.RenderHalo` : Whether to render the lens halo.
- `r.PrettyPostProcess.RenderGlare` : Whether to render the glare strokes.

# FAQ

### Can I use this with Unreal Engine I got from Epic Games Launcher?

No. This plugin requires engine modification and won't work with vanilla launcher builds. Installing the Source Code component only includes engine headers for your
project's C++ code to compile with, and are read only.

### Where's version 1.x? Why start on 2.x?

This implementation goes back since Froyok's article on custom lens flares was first published. Version 1.x was used with UE4, and version 2.x is used with UE5.
We used it way before we got the blessings from Froyok to make this an open source plugin and after receiving few requests to do so.

### Is it compatible with UE4?

As long as you apply the engine changes [mentioned in the article](https://www.froyok.fr/blog/2021-12-ue4-custom-bloom/page.html), yes, it is compatible with
UE4.26 or later. The plugin codes have preprocessor definitions that accomodate for UE5 codebase changes, but you need to convert `TObjectPtr` types into raw
pointers, as it is mandated in UE5.

However, .uasset files in the Content folder was saved with UE5.1, and will not show up in prior versions. In that case, you will need to make the assets yourself.

> **NOTE:** If you're making pull request, be sure not to commit anything in the Contents folder. This ensures compatibility with older versions down to UE5.1 and
> allowing for updates.

### The lens flares doesn't work and the Data Asset doesn't show up in the Content Browser. What gives?

The Data Asset asset was made with UE5.1, and along with other assets in Unreal Engine, it is not backwards compatible, including UE5.0.

### Why not using Developer Settings?

Using it as opposed to Data Asset would be more ideal. However, it crashes the editor when modifying the value from Dev settings. To avoid modifying the engine
further, it is settled to use Data Asset.

# License

This plugin is licensed under BSD-3-Clause Clear. See LICENSE file for more information.



