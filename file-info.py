'''
Show metadata stored in a file produced by the FCCAnalyses.
'''

import os
import sys

from typing import Optional

import ROOT


def get_string(directory: ROOT.TDirectory,
               obj_name: str) -> Optional[int]:
    '''
    Get string value from a TNamed in the TDirectory.
    '''

    try:
        string_value: str = directory[obj_name].GetTitle()
    except KeyError:
        return None

    if not isinstance(string_value, str):
        return None

    return string_value


def get_int(directory: ROOT.TDirectory,
            parameter_name: str) -> Optional[int]:
    '''
    Get integer value from a TParameter in the TDirectory.
    '''

    try:
        parameter_value: int = directory[parameter_name].GetVal()
    except KeyError:
        return None

    if not isinstance(parameter_value, int):
        return None

    return parameter_value


def get_float(directory: ROOT.TDirectory,
              parameter_name: str) -> Optional[float]:
    '''
    Get float value from a TParameter in the TDirectory.
    '''

    try:
        parameter_value: float = directory[parameter_name].GetVal()
    except KeyError:
        return None

    if not isinstance(parameter_value, float):
        return None

    return parameter_value


def file_info(filepath: str) -> None:
    '''
    Print metadata stored in the file produced by the FCCAnalyses.
    '''

    if not os.path.isfile(filepath):
        print('ERROR: Provided file path appears not to be a '
              'file!\nAborting...')
        sys.exit(3)

    info_msg: str = 'Provided file contains the following metadata:'
    n_failures: int = 0

    with ROOT.TFile(filepath, 'READ') as infile:
        try:
            fccana_dir = infile['fccana']
        except KeyError:
            print('ERROR: Can\'t find metadata directory!\n'
                  'Are you sure this file was produced by FCCAnalyses?\n'
                  'Aborting...')
            sys.exit(3)

        # Get who processed the file
        processed_with = get_string(fccana_dir, 'processed-with')
        if processed_with is None:
            print('ERROR: Could not determine who processed the file!\n'
                  'File might be corrupted...')
            n_failures += 1
        else:
            info_msg += '\n - processed with:  ' \
                        f'                                    {processed_with}'

        # Original number of events / sum of weights
        n_events_orig = get_int(fccana_dir, 'n-events-original')
        info_msg += '\n - original number of events in the chunk:  '
        if n_events_orig is None:
            info_msg += '            Unknown'
        else:
            info_msg += f'            {n_events_orig:,}'

        sow_orig = get_float(fccana_dir, 'sow-original')
        # if sow_orig is not None:
        info_msg += '\n - original sum of weights in the chunk:  '
        if sow_orig is None:
            info_msg += '              Unknown'
        else:
            info_msg += f'              {sow_orig:0,.2g}'

        # Initial number of events / sum of weights
        n_events_init = get_int(fccana_dir, 'n-events-initial')
        if n_events_init is None:
            print('ERROR: Could not determine initial number of events!\n'
                  'File might be corrupted...')
            n_failures += 1
        else:
            info_msg += '\n - initial number of events in the chunk:  ' \
                        f'             {n_events_init:,}'

        sow_init = get_float(fccana_dir, 'sow-initial')
        if sow_init is None:
            print('ERROR: Could not determine initial sum of weights!\n'
                  'File might be corrupted...')
            n_failures += 1
        else:
            info_msg += '\n - initial sum of weights in the chunk:  ' \
                        f'               {sow_init:0,.2g}'

        if n_failures > 2:
            print('ERROR: Corrupted file encountered!\nAborting...')
            sys.exit(3)

        # Restricted number of events / sum of weights
        n_events_restricted = get_int(fccana_dir, 'n-events-restricted')
        if n_events_restricted is None:
            print('ERROR: Could not determine restricted number of initial '
                  'events!\n'
                  'File might be corrupted...')
            n_failures += 1
        else:
            info_msg += '\n - restricted number of initial events in the ' \
                        'chunk:  ' \
                        f'  {n_events_restricted:,}'

        sow_restricted = get_float(fccana_dir, 'sow-restricted')
        if sow_restricted is None:
            print('ERROR: Could not determine restricted sum of the initial '
                  'weights!\n'
                  'File might be corrupted...')
            n_failures += 1
        else:
            info_msg += '\n - restricted sum of the initial weights in the ' \
                        'chunk:  ' \
                        f'{sow_restricted:0,.2g}'

        if n_failures > 2:
            print('ERROR: Corrupted file encountered!\nAborting...')
            sys.exit(3)

        # Final number of events / sum of weights
        n_events_final = get_int(fccana_dir, 'n-events-final')
        if n_events_final is None:
            print('ERROR: Could not determine final number of events!\n'
                  'File might be corrupted...')
            n_failures += 1
        else:
            info_msg += '\n - final number of events in the chunk:  ' \
                        f'               {n_events_final:,}'

        sow_final = get_float(fccana_dir, 'sow-final')
        if sow_final is None:
            print('ERROR: Could not determine final sum of weights!\n'
                  'File might be corrupted...')
            n_failures += 1
        else:
            info_msg += '\n - final sum of weights in the chunk:  ' \
                        f'                 {sow_final:0,.2g}'

        if n_failures > 2:
            print('ERROR: Corrupted file encountered!\nAborting...')
            sys.exit(3)

    print(info_msg)


if __name__ == '__main__':
    FILEPATH = 'fff.root'

    file_info(FILEPATH)
