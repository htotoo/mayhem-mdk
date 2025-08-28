#
# Copyright (C) 2024 Bernd Herzog
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
	build-essential \
	make cmake \
	git gnupg2 \
	xz-utils curl \
	gcc-arm-none-eabi binutils-arm-none-eabi \
	clang-format clang-tidy && mkdir /build
