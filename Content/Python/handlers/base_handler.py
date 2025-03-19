from abc import ABC, abstractmethod
from typing import Dict, Any


class CommandHandler(ABC):
    """
    Abstract base class for command handlers
    
    All command handlers should inherit from this class and implement the handle method
    """

    @abstractmethod
    def handle(self, command: Dict[str, Any]) -> Dict[str, Any]:
        """
        Handle a command and return a response
        
        Args:
            command: The command dictionary
            
        Returns:
            A response dictionary with at least a 'success' key
        """
        pass