/*
 * file_storage.h
 *
 * Copyright (C) 2010 Amazon Technologies, Inc.  All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

/* Fake file storage gadget for charging.  Calling file_storage_enable()
 * causes the device to enumerate on the host so we can draw 500 mA for
 * charging.  
 */

#ifndef __FILE_STORAGE_H__
#define __FILE_STORAGE_H__

int file_storage_enable(int exit_voltage);

#endif
