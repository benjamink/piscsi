//---------------------------------------------------------------------------
//
// SCSI Target Emulator PiSCSI
// for Raspberry Pi
//
// Copyright (C) 2021-2022 Uwe Seimet
//
//---------------------------------------------------------------------------

#include "shared/piscsi_util.h"
#include "shared/protobuf_util.h"
#include "shared/piscsi_exceptions.h"
#include "scsictl_commands.h"
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <list>

using namespace std;
using namespace piscsi_interface;
using namespace piscsi_util;
using namespace protobuf_util;

bool ScsictlCommands::Execute(const string& log_level, const string& default_folder, const string& reserved_ids,
		const string& image_params, const string& filename)
{
	switch(command.operation()) {
		case LOG_LEVEL:
			return CommandLogLevel(log_level);

		case DEFAULT_FOLDER:
			return CommandDefaultImageFolder(default_folder);

		case RESERVE_IDS:
			return CommandReserveIds(reserved_ids);

		case CREATE_IMAGE:
			return CommandCreateImage(image_params);

		case DELETE_IMAGE:
			return CommandDeleteImage(image_params);

		case RENAME_IMAGE:
			return CommandRenameImage(image_params);

		case COPY_IMAGE:
			return CommandCopyImage(image_params);

		case DEVICES_INFO:
			return CommandDeviceInfo();

		case DEVICE_TYPES_INFO:
			return CommandDeviceTypesInfo();

		case VERSION_INFO:
			return CommandVersionInfo();

		case SERVER_INFO:
			return CommandServerInfo();

		case DEFAULT_IMAGE_FILES_INFO:
			return CommandDefaultImageFilesInfo();

		case IMAGE_FILE_INFO:
			return CommandImageFileInfo(filename);

		case NETWORK_INTERFACES_INFO:
			return CommandNetworkInterfacesInfo();

		case LOG_LEVEL_INFO:
			return CommandLogLevelInfo();

		case RESERVED_IDS_INFO:
			return CommandReservedIdsInfo();

		case MAPPING_INFO:
			return CommandMappingInfo();

		case OPERATION_INFO:
			return CommandOperationInfo();

		default:
			return SendCommand();
	}

    return false;
}

bool ScsictlCommands::SendCommand()
{
	sockaddr_in server_addr = {};
	if (!ResolveHostName(hostname, &server_addr)) {
		throw io_exception("Can't resolve hostname '" + hostname + "'");
	}

	const int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		throw io_exception("Can't create socket: " + string(strerror(errno)));
	}

	server_addr.sin_port = htons(uint16_t(port));
	if (connect(fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		close(fd);

		throw io_exception("Can't connect to piscsi on host '" + hostname + "', port " + to_string(port)
				+ ": " + strerror(errno));
	}

	if (write(fd, "RASCSI", 6) != 6) {
		close(fd);

		throw io_exception("Can't write magic");
	}

	serializer.SerializeMessage(fd, command);
    serializer.DeserializeMessage(fd, result);

    close(fd);

    if (!result.status()) {
    	throw io_exception(result.msg());
    }

	if (!result.msg().empty()) {
		cout << result.msg() << endl;
	}

	return true;
}

bool ScsictlCommands::CommandDevicesInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayDevicesInfo(result.devices_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandLogLevel(const string& log_level)
{
	SetParam(command, "level", log_level);

	return SendCommand();
}

bool ScsictlCommands::CommandReserveIds(const string& reserved_ids)
{
	SetParam(command, "ids", reserved_ids);

	return SendCommand();
}

bool ScsictlCommands::CommandCreateImage(const string& image_params)
{
	if (const size_t separator_pos = image_params.find(COMPONENT_SEPARATOR); separator_pos != string::npos) {
		SetParam(command, "file", string_view(image_params).substr(0, separator_pos));
		SetParam(command, "size", string_view(image_params).substr(separator_pos + 1));
	}
	else {
		cerr << "Error: Invalid file descriptor '" << image_params << "', format is NAME:SIZE" << endl;

		return false;
	}

	SetParam(command, "read_only", "false");

	return SendCommand();
}

bool ScsictlCommands::CommandDeleteImage(const string& filename)
{
	SetParam(command, "file", filename);

	return SendCommand();
}

bool ScsictlCommands::CommandRenameImage(const string& image_params)
{
	if (const size_t separator_pos = image_params.find(COMPONENT_SEPARATOR); separator_pos != string::npos) {
		SetParam(command, "from", string_view(image_params).substr(0, separator_pos));
		SetParam(command, "to", string_view(image_params).substr(separator_pos + 1));
	}
	else {
		cerr << "Error: Invalid file descriptor '" << image_params << "', format is CURRENT_NAME:NEW_NAME" << endl;

		return false;
	}

	return SendCommand();
}

bool ScsictlCommands::CommandCopyImage(const string& image_params)
{
	if (const size_t separator_pos = image_params.find(COMPONENT_SEPARATOR); separator_pos != string::npos) {
		SetParam(command, "from", string_view(image_params).substr(0, separator_pos));
		SetParam(command, "to", string_view(image_params).substr(separator_pos + 1));
	}
	else {
		cerr << "Error: Invalid file descriptor '" << image_params << "', format is CURRENT_NAME:NEW_NAME" << endl;

		return false;
	}

	return SendCommand();
}

bool ScsictlCommands::CommandDefaultImageFolder(const string& folder)
{
	SetParam(command, "folder", folder);

	return SendCommand();
}

bool ScsictlCommands::CommandDeviceInfo()
{
	SendCommand();

	for (const auto& device : result.devices_info().devices()) {
		cout << scsictl_display.DisplayDeviceInfo(device);
	}

	cout << flush;

	return true;
}

bool ScsictlCommands::CommandDeviceTypesInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayDeviceTypesInfo(result.device_types_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandVersionInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayVersionInfo(result.version_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandServerInfo()
{
	SendCommand();

	PbServerInfo server_info = result.server_info();

	cout << scsictl_display.DisplayVersionInfo(server_info.version_info());
	cout << scsictl_display.DisplayLogLevelInfo(server_info.log_level_info());
	cout << scsictl_display.DisplayImageFilesInfo(server_info.image_files_info());
	cout << scsictl_display.DisplayMappingInfo(server_info.mapping_info());
	cout << scsictl_display.DisplayNetworkInterfaces(server_info.network_interfaces_info());
	cout << scsictl_display.DisplayDeviceTypesInfo(server_info.device_types_info());
	cout << scsictl_display.DisplayReservedIdsInfo(server_info.reserved_ids_info());
	cout << scsictl_display.DisplayOperationInfo(server_info.operation_info());

	if (server_info.devices_info().devices_size()) {
		list<PbDevice> sorted_devices = { server_info.devices_info().devices().begin(), server_info.devices_info().devices().end() };
		sorted_devices.sort([](const auto& a, const auto& b) { return a.id() < b.id() || a.unit() < b.unit(); });

		cout << "Attached devices:\n";

		for (const auto& device : sorted_devices) {
			cout << scsictl_display.DisplayDeviceInfo(device);
		}
	}

	cout << flush;

	return true;
}

bool ScsictlCommands::CommandDefaultImageFilesInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayImageFilesInfo(result.image_files_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandImageFileInfo(const string& filename)
{
	SetParam(command, "file", filename);

	SendCommand();

	cout << scsictl_display.DisplayImageFile(result.image_file_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandNetworkInterfacesInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayNetworkInterfaces(result.network_interfaces_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandLogLevelInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayLogLevelInfo(result.log_level_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandReservedIdsInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayReservedIdsInfo(result.reserved_ids_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandMappingInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayMappingInfo(result.mapping_info()) << flush;

	return true;
}

bool ScsictlCommands::CommandOperationInfo()
{
	SendCommand();

	cout << scsictl_display.DisplayOperationInfo(result.operation_info()) << flush;

	return true;
}

bool ScsictlCommands::ResolveHostName(const string& host, sockaddr_in *addr)
{
	addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (addrinfo *result; !getaddrinfo(host.c_str(), nullptr, &hints, &result)) {
		*addr = *(sockaddr_in *)(result->ai_addr);
		freeaddrinfo(result);
		return true;
	}

	return false;
}
