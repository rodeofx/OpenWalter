# Copyright 2017 Rodeo FX. All rights reserved.

class MergeExpressions:
    """
    Merge two words to the maximal expression that matches to both objects.
    The algorithm is based on the Longest Common Subsequence problem and the
    implementation at
    wikibooks.org/wiki/Algorithm_Implementation/Strings/Longest_common_subsequence
    """
    def __init__(self, first, second):
        """
        Called after the instance has been created. Computing and caching the
        length table for the Longest Common Subsequence problem.

        :param string first: The first string.
        :param string first: The second string.
        """
        self.first = first
        self.second = second

        # Computing the length table of the LCS
        self.firstLen = len(first)
        self.secondLen = len(second)

        # An (firstLen+1) times (secondLen+1) matrix
        self.table = \
            [[0] * (self.secondLen + 1) for _ in range(self.firstLen + 1)]
        for i in range(1, self.firstLen + 1):
            for j in range(1, self.secondLen + 1):
                if self.first[i-1] == self.second[j-1]:
                    self.table[i][j] = self.table[i-1][j-1] + 1
                else:
                    self.table[i][j] = \
                        max(self.table[i][j-1], self.table[i-1][j])

    def diff(self, i, j):
        """
        Compare sub-strings and return the expression that matches both parts of
        the sub-strings.

        :param int i: The number of the symbol of the first string to compare.
        :param int j: The number of the symbol of the second string to compare.
        :returns: The expression that matches both parts of the sub-strings.
        :rtype: string
        """
        if i > 0 and j > 0 and self.first[i-1] == self.second[j-1]:
            # Match
            return self.diff(i - 1, j - 1) + self.first[i-1]
        else:
            # Mismatch
            if j > 0 and (i == 0 or self.table[i][j-1] >= self.table[i-1][j]):
                # We need to add a letter second[j-1] to the sequence
                expression = self.diff(i, j - 1)
            elif i > 0 and (j == 0 or self.table[i][j-1] < self.table[i-1][j]):
                # We need to remove a letter first[i-1] to the sequence
                expression = self.diff(i - 1, j)
            else:
                # The recursion is finished
                return ""

            # A small trick to avoid producing several stars in a row.
            if not expression or expression[-1] != "*":
                expression += ".*"

            return expression

    def __call__(self):
        """
        Compare saved strings and return the expression that matches both
        strings.

        :returns: The expression that matches both strings.
        :rtype: string
        """
        return self.diff(self.firstLen, self.secondLen)
