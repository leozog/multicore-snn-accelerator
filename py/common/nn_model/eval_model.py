import torch

def eval_model(device, model, criterion, loader):
    model.eval()
    total_loss = 0.0
    correct = 0
    total = 0
    with torch.no_grad():
        for img, labels in loader:
            try:
                img = img.to(device, non_blocking=True)
                labels = labels.to(device, non_blocking=True)
                outputs = model(img)
                loss = criterion(outputs, labels)
                predict = outputs.argmax(dim=1)
                correct += (predict == labels).sum().item()
                total_loss += loss.item() * labels.size(0)
                total += labels.size(0)
            except:
                pass
    epoch_loss = total_loss / total
    epoch_acc = correct / total
    return epoch_loss, epoch_acc