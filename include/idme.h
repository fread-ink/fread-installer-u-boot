/*
 * idme.h 
 *
 * Copyright 2010 Amazon Technologies, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file idme.h
 * @brief This file contains functions for interacting with variables
 *	in the userstore partition
 *
 */

#ifndef __IDME_H__
#define __IDME_H__

int idme_check_update(void);
int idme_get_var(const char *name, char *buf, int buflen);
int idme_update_var(const char *name, const char *value);

#endif /* __IDME_H__ */
