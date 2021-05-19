/**
 * @file AsciiRasterInterface.h
 * @author Karsten Rink
 * @date 2014-09-10
 * @brief Definition of the AsciiRasterInterface class.
 *
 * @copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#pragma once

#include <fstream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "GeoLib/Raster.h"

namespace FileIO
{
/**
 * Interface for reading and writing a number of ASCII raster formats.
 * Currently supported are reading and writing of Esri asc-files and
 * reading of Surfer grd-files.
 */
class AsciiRasterInterface {
public:
    /// Reads raster file by detecting type based on extension and then calling the appropriate method
    static GeoLib::Raster* readRaster(std::string const& fname);

    /// Reads an ArcGis ASC raster file
    static GeoLib::Raster* getRasterFromASCFile(std::string const& fname);

    /// Reads a Surfer GRD raster file
    static GeoLib::Raster* getRasterFromSurferFile(std::string const& fname);

    /// Writes an Esri asc-file
    static void writeRasterAsASC(GeoLib::Raster const& raster, std::string const& file_name);


private:
    /// Reads the header of a Esri asc-file.
    /// If the return value is empty, reading was not successful.
    static std::optional<GeoLib::RasterHeader> readASCHeader(std::ifstream& in);

    /// Reads the header of a Surfer grd-file with minimum and maximum values.
    /// If the return value is empty, reading was not successful.
    static std::optional<std::tuple<GeoLib::RasterHeader, double, double>>
    readSurferHeader(std::ifstream& in);
};

/// Reads a vector of rasters given by file names. On error nothing is returned,
/// otherwise the returned vector contains pointers to the read rasters.
std::optional<std::vector<GeoLib::Raster const*>> readRasters(
    std::vector<std::string> const& raster_paths);
} // end namespace FileIO
